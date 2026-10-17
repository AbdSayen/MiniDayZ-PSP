const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
const prefixToSheet = (p) => p + '-sheet0';
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

const defAt = raw.indexOf(
  '[180,40,null,2,false,false,false,418174823361724,false,[[1,[2,"create_map_location"]]]]],[],[['
);
console.log('primary def', defAt);
const body = raw.slice(defAt, defAt + 1_800_000);

const layoutNames = [
  'village', 'village2', 'village3', 'village4',
  'city', 'city2', 'city3',
  'barrack', 'military', 'military2',
  'gas', 'gas2',
  'hospital',
  'firestation', 'firestation2',
  'police', 'police2',
  'church',
  'helicrash', 'secret', 'hunter',
];

function extractLayout(name) {
  // Compare string: [7,[2,"city"]] or [8,0],[7,[2,"city"]]
  const needles = [
    `[7,[2,"${name}"]]`,
    `[2,"${name}"]],[8,0]`,
  ];
  let i = -1;
  for (const n of needles) {
    i = body.indexOf(n);
    if (i >= 0) break;
  }
  if (i < 0) return null;
  const chunk = body.slice(i, i + 80000);

  const spawns = [];
  // Create object: [-1,94,null ... [4,OID] ... coords
  // Also System Create object by index: look for plugin 19 types via [0,IDX] after action 94
  const parts = chunk.split('[-1,94,null');
  for (let pi = 1; pi < parts.length; pi++) {
    const p = parts[pi].slice(0, 1200);
    let objIdx = null;
    const oi = p.match(/\[4,(\d+)\]/);
    if (oi) objIdx = +oi[1]; // uid not idx
    // offsets from hotspot 507
    let dx = null,
      dy = null;
    const xm = p.match(/\[0,\[4,\[20,507,65,false,null\],\[0,(-?\d+)\]\]\]/);
    const ym = p.match(/\[0,\[4,\[20,507,66,false,null\],\[0,(-?\d+)\]\]\]/);
    const xm2 = p.match(/\[0,\[5,\[20,507,65,false,null\],\[0,(-?\d+)\]\]\]/);
    const ym2 = p.match(/\[0,\[5,\[20,507,66,false,null\],\[0,(-?\d+)\]\]\]/);
    if (xm) dx = +xm[1];
    else if (xm2) dx = +xm2[1];
    if (ym) dy = +ym[1];
    else if (ym2) dy = +ym2[1];
    // absolute [0,[0,N]]
    if (dx == null) {
      const am = p.match(/\[0,\[0,(-?\d+)\]\],\s*\[0,\[0,(-?\d+)\]\]/);
      if (am) {
        dx = +am[1];
        dy = +am[2];
      }
    }
    // object type index in spawn params [0,321]
    let typeIdx = null;
    for (const m of p.matchAll(/\[0,(\d{3})\]/g)) {
      const idx = +m[1];
      if (idxToPrefix[idx] && idx >= 308 && idx <= 395) {
        typeIdx = idx;
        break;
      }
    }
    if (typeIdx != null) {
      spawns.push({
        prefix: idxToPrefix[typeIdx],
        dx: dx ?? 0,
        dy: dy ?? 0,
      });
    }
  }

  // Dedupe by prefix+offset
  const key = (s) => s.prefix + '@' + s.dx + ',' + s.dy;
  const seen = new Set();
  const uniq = [];
  for (const s of spawns) {
    const k = key(s);
    if (seen.has(k)) continue;
    seen.add(k);
    uniq.push(s);
  }
  return uniq;
}

for (const name of layoutNames) {
  const s = extractLayout(name);
  if (!s || !s.length) continue;
  console.log('\n##', name, '(', s.length, 'spawns)');
  for (const x of s) console.log(' ', x.prefix, x.dx, x.dy);
}

// Fallback: search any [2,"village"] in body
const vi = body.indexOf('[2,"village"]');
console.log('\nvillage string at', vi);
