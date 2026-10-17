const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

const defAt = raw.indexOf(
  '[180,40,null,2,false,false,false,7004939855155178,false,[[1,[2,"create_map_element"]]]]],[],[['
);
console.log('defAt', defAt);
const body = raw.slice(defAt, defAt + 3_500_000);

// Compare branches on function param 151 (map element type)
// Pattern: [7,[20,501,151,false,null]] or [8,0],[7,[0,N]]
const branches = [];
const re = /\[8,0\],\[7,\[0,(\d+)\]\]\]\]\],/g;
let m;
while ((m = re.exec(body)) !== null) {
  // look back for compare variable
  const back = body.slice(Math.max(0, m.index - 120), m.index + 40);
  if (back.includes('501,151') || back.includes('507,151')) {
    branches.push(+m[1]);
  }
}
console.log('branches on 501/507.151:', [...new Set(branches)].sort((a, b) => a - b));

// For each branch value, find chunk until next branch and extract spawns
// Spawn pattern: [-1,94,null with coords using hotspot + offset
// Simpler: find all sequences: compare type T then nearby object idx [0,312] etc

function chunkForType(typeId) {
  const marker = `[7,[20,501,151,false,null]]],[8,0],[7,[0,${typeId}]]`;
  let i = body.indexOf(marker);
  if (i < 0) {
    const m2 = `[7,[20,507,151,false,null]]],[8,0],[7,[0,${typeId}]]`;
    i = body.indexOf(m2);
  }
  if (i < 0) return '';
  return body.slice(i, i + 25000);
}

for (const t of [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]) {
  const ch = chunkForType(t);
  if (!ch) continue;
  const objs = [];
  for (const mm of ch.matchAll(/\[0,(\d{2,3})\]/g)) {
    const idx = +mm[1];
    if (idxToPrefix[idx] && idx >= 308 && idx <= 395) objs.push(idxToPrefix[idx]);
  }
  const off = [];
  for (const mm of ch.matchAll(/\[0,\[4,\[20,507,65,false,null\],\[0,(-?\d+)\]\]\]/g)) off.push(+mm[1]);
  for (const mm of ch.matchAll(/\[0,\[5,\[20,507,65,false,null\],\[0,(-?\d+)\]\]\]/g)) off.push(+mm[1]);
  if (objs.length) {
    console.log('\nType', t, 'buildings:', [...new Set(objs)].join(', '));
    console.log('  x-offset samples:', off.slice(0, 12));
  }
}

// Also dump object idx frequency in full body
const freq = {};
for (const mm of body.matchAll(/\[0,(\d{3})\]/g)) {
  const idx = +mm[1];
  if (idxToPrefix[idx] && idx >= 308 && idx <= 395) freq[idx] = (freq[idx] || 0) + 1;
}
console.log('\nFull body building idx freq:');
Object.entries(freq)
  .sort((a, b) => b[1] - a[1])
  .forEach(([k, v]) => console.log(k, idxToPrefix[k], v));
