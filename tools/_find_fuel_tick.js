const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, pred, out) {
  if (!Array.isArray(evts)) return;
  if (pred(JSON.stringify(evts), evts)) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, pred, out);
}

const out = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s, e) =>
      s.includes('Drivable vehicles') &&
      s.includes('"fuel"') &&
      (s.includes('0.1') || s.includes('Every')),
    out
  );
});
console.log('drivable fuel every', out.length);

const out2 = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s) => s.includes(',946,284') && s.includes('0.1'),
    out2
  );
});
console.log('946+284+0.1', out2.length);
out2.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
if (out2[0]) console.log(JSON.stringify(out2[0]).slice(0, 2000));
