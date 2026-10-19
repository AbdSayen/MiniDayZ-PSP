const fs = require('fs');
const path = require('path');
const fn = process.argv[2];
const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8').replace(/^\uFEFF/, '')
).project;

function isRoot(e) {
  return (
    Array.isArray(e) &&
    e.length >= 7 &&
    e[0] === 0 &&
    Array.isArray(e[1]) &&
    e[1][0] === 180 &&
    e[1][1] === 40 &&
    JSON.stringify(e).includes('"' + fn + '"')
  );
}

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (isRoot(e)) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log('roots', hits.length, hits.map((h) => JSON.stringify(h).length).join(','));
const pick = hits[hits.length - 1];
const outPath = path.join(__dirname, '..', 'cpp', 'docs_gen', 'functions', fn + '.json');
fs.writeFileSync(outPath, JSON.stringify(pick, null, 2));
console.log('wrote', outPath, 'len', JSON.stringify(pick).length);
