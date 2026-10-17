const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const analysis = JSON.parse(
  fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/c2_analysis.json'), 'utf8')
);

const idxToPrefix = {};
for (const o of analysis.objects || analysis.objectTypes || []) {
  if (o.idx != null && o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}
// c2_analysis uses "objects" key - verify
const objs = analysis.objects || [];
for (const o of objs) {
  if (o.idx >= 300 && o.idx <= 370 && o.prefixes) {
    idxToPrefix[o.idx] = o.prefixes[0];
  }
}

function sliceFn(name, maxBytes) {
  const i = raw.indexOf('"' + name + '"');
  if (i < 0) return '';
  return raw.slice(i, i + maxBytes);
}

// create_map_location dispatches on location type — find string compares
const locWin = sliceFn('create_map_location', 3_000_000);
console.log('create_map_location window', locWin.length);

const locTypes = [
  'village', 'city', 'military', 'gas', 'hospital', 'fire', 'police', 'church',
  'helicrash', 'bunker', 'secret', 'hunter',
];
for (const t of locTypes) {
  const re = new RegExp('"' + t + '"', 'g');
  let c = 0;
  while (re.exec(locWin)) c++;
  if (c) console.log(' type', t, c);
}

// For each type, find a window after the string and collect nearby [0,objIdx] and coordinate literals
function extractLayouts(typeName) {
  const marker = '"' + typeName + '"';
  let pos = 0;
  const layouts = [];
  while (true) {
    const i = locWin.indexOf(marker, pos);
    if (i < 0) break;
    const chunk = locWin.slice(i, i + 12000);
    const spawns = [];
    // C2 spawn often: ... [0,objType], ... offset pairs as [0,x],[0,y] or [7,[0,x]]
    for (const mm of chunk.matchAll(/\[0,(\d{3})\]/g)) {
      const idx = +mm[1];
      if (idx >= 308 && idx <= 360 && idxToPrefix[idx]) {
        spawns.push({ idx, prefix: idxToPrefix[idx], at: mm.index });
      }
    }
    // grab numeric literals in [-800,800] near each spawn
    const entries = [];
    for (const s of spawns.slice(0, 40)) {
      const sub = chunk.slice(Math.max(0, s.at - 200), s.at + 400);
      const nums = [...sub.matchAll(/\[0,(-?\d+)\]/g)]
        .map((x) => +x[1])
        .filter((n) => n !== s.idx && Math.abs(n) <= 900 && n !== 0);
      entries.push({ idx: s.idx, prefix: s.prefix, nums: nums.slice(-6) });
    }
    if (entries.length >= 2) layouts.push(entries);
    pos = i + marker.length;
  }
  return layouts;
}

for (const t of ['village', 'city', 'military', 'gas', 'hospital', 'fire', 'police', 'church']) {
  const layouts = extractLayouts(t);
  console.log('\n===', t, 'blocks with 2+ spawns:', layouts.length);
  if (layouts.length) {
    const best = layouts.reduce((a, b) => (a.length >= b.length ? a : b));
    console.log(JSON.stringify(best, null, 2));
  }
}

console.log('\nIdx map 308-360:');
for (let i = 308; i <= 360; i++) {
  if (idxToPrefix[i]) console.log(i, idxToPrefix[i]);
}
