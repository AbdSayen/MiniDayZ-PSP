const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  const s = JSON.stringify(evts);
  if (s.includes('"Vehicle"') && (s.includes(',284') || s.includes('946,284'))) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, out);
}

const out = [];
project[6].forEach((sh) => walk(sh[1], out));
out.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
for (let i = 0; i < 8; i++) {
  console.log('---', i, 'len', JSON.stringify(out[i]).length);
  console.log(JSON.stringify(out[i]));
}
