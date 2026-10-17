const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, out, depth) {
  if (!Array.isArray(evts) || depth > 60) return;
  const s = JSON.stringify(evts);
  // Set/Add on Vehicle IV fuel (284) or global fuel string
  if (
    s.includes('"Vehicle"') &&
    s.includes('284') &&
    (s.includes('"fuel"') || s.includes(',284,')) &&
    (s.includes('"subtract"') || s.includes(',11,') || s.includes('[10,'))
  ) {
    if (s.length < 5000) out.push(evts);
  }
  for (const e of evts) if (Array.isArray(e)) walk(e, out, depth + 1);
}

const out = [];
project[6].forEach((sh) => walk(sh[1], out, 0));
out.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log('count', out.length);
for (let i = 0; i < Math.min(10, out.length); i++) {
  console.log('len', JSON.stringify(out[i]).length, JSON.stringify(out[i]).slice(0, 800));
}
