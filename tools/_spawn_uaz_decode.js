const fs = require('fs');
const project = JSON.parse(fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')).project;
const ot = project[3].find(o => o[0] === 't806');
console.log('t806 length', ot.length);
for (let i = 0; i < ot.length; i++) {
  const v = ot[i];
  if (typeof v === 'string' || typeof v === 'number' || typeof v === 'boolean') {
    console.log(i, typeof v, v);
  } else if (Array.isArray(v)) {
    console.log(i, 'array len', v.length, 'sample', JSON.stringify(v).slice(0, 200));
  } else console.log(i, typeof v);
}

// project[4] is often instance variable types
console.log('\nproject[4] sample:', JSON.stringify(project[4]).slice(0, 500));

// Walk project[4] or ot for IV name arrays
function findIvDefs(node, path) {
  if (!Array.isArray(node)) return;
  if (node.length >= 3 && node[0] === 0 && typeof node[1] === 'string' && typeof node[2] === 'number') {
    console.log('IV def at', path, ':', node[1], 'type', node[2], 'def', node[3]);
  }
  for (let i = 0; i < node.length; i++) findIvDefs(node[i], path + '[' + i + ']');
}
console.log('\nIV defs in t806:');
findIvDefs(ot, 'ot');

// Extract Car_respawn_uaz body len ~3330
function walk(evts, fn, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('"' + fn + '"') && s.includes('806')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, fn, out);
  }
}
const hits = [];
project[6].forEach(sh => walk(sh[1], 'Car_respawn_uaz', hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
const best = hits[0];
fs.writeFileSync('cpp/docs_gen/functions/Car_respawn_uaz_body.json', JSON.stringify(best, null, 2));
console.log('\nWrote Car_respawn_uaz_body.json len', JSON.stringify(best).length);
