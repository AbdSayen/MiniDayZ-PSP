const fs = require('fs');
const s = fs.readFileSync('original_docs/data.js', 'utf8');
// find all unique tokens containing uaz or volga or truck near car_
const re = /[a-z0-9_]*(?:uaz|volga|truck|car_resp)[a-z0-9_]*/gi;
const set = new Set();
let m;
while ((m = re.exec(s))) set.add(m[0]);
console.log([...set].sort().join('\n'));

// Extract object type entry: search for patterns like ["something_uaz"
const idx = s.indexOf('uaz');
console.log('\nfirst uaz context:\n', s.slice(idx - 120, idx + 800));

// Parse project object types from index 7 (415 items)
const j = JSON.parse(s.slice(s.indexOf('{')));
for (let pi = 0; pi < j.project.length; pi++) {
  const p = j.project[pi];
  if (!Array.isArray(p)) continue;
  for (const ot of p) {
    if (!Array.isArray(ot) || typeof ot[0] !== 'string') continue;
    if (!/uaz|volga|truck|bi_car/i.test(ot[0])) continue;
    console.log('\n=== OT', ot[0], 'at project', pi, '===');
    // instance variables usually at index 17 or search nested
    function findIVs(node, depth) {
      if (depth > 8 || !node) return;
      if (Array.isArray(node) && node.length >= 4 && node[0] === 0 && typeof node[1] === 'string' && typeof node[3] === 'number') {
        console.log(' IV', node[1], 'default', node[3]);
      }
      if (Array.isArray(node)) for (const c of node) findIVs(c, depth + 1);
    }
    findIVs(ot, 0);
  }
}
