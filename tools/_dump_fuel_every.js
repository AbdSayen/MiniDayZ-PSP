const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, pred, out) {
  if (!Array.isArray(evts)) return;
  const s = JSON.stringify(evts);
  if (pred(s, evts)) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, pred, out);
}

const out = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s) => s.includes('946,88') && s.includes('284') && s.includes('0.1'),
    out
  );
});
out.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log(JSON.stringify(out[0], null, 2));
