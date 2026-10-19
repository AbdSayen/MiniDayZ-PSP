const fs = require('fs');
const path = require('path');
const fn = process.argv[2];
const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (let i = 0; i < evts.length; i++) {
    const e = evts[i];
    if (!Array.isArray(e)) continue;
    const conds = e[5];
    if (Array.isArray(conds)) {
      for (const c of conds) {
        if (!Array.isArray(c)) continue;
        if (c[0] === 180 && c[1] === 40) {
          const pars = c[9];
          const s = JSON.stringify(pars);
          if (s && s.includes('"' + fn + '"')) out.push(e);
        }
      }
    }
    for (let j = 0; j < e.length; j++) {
      const x = e[j];
      if (Array.isArray(x)) walk(x, out);
    }
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log(fn, 'roots', hits.length, hits.map((h) => JSON.stringify(h).length).join(','));
const pick = hits[hits.length - 1];
if (pick) {
  fs.writeFileSync(
    path.join(__dirname, '..', 'cpp', 'docs_gen', 'functions', fn + '.json'),
    JSON.stringify(pick, null, 2)
  );
}
