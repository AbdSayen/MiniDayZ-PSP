/** Extract smallest event subtree containing a function name string. */
const fs = require('fs');
const path = require('path');
const fn = process.argv[2];
if (!fn) {
  console.error('usage: node _extract_fn_tree.js <name>');
  process.exit(1);
}
const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes('"' + fn + '"')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log('hits', hits.length);
hits.slice(0, 8).forEach((h, i) => console.log(i, JSON.stringify(h).length));

const pick =
  hits.find((h) => {
    const l = JSON.stringify(h).length;
    return l > 500 && l < 8000;
  }) || hits[0];
if (!pick) {
  console.log('no hit');
  process.exit(1);
}

const outPath = path.join(__dirname, '..', 'cpp', 'docs_gen', 'functions', fn + '.json');
fs.writeFileSync(outPath, JSON.stringify(pick, null, 2));
console.log('wrote', outPath, 'len', JSON.stringify(pick).length);
