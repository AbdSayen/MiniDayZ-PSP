const fs = require('fs');
const project = JSON.parse(fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')).project;
const fn = process.argv[2];
function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (JSON.stringify(e).includes('"' + fn + '"')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}
const hits = [];
project[6].forEach(sh => walk(sh[1], hits));
hits.sort((a,b)=>JSON.stringify(a).length-JSON.stringify(b).length);
fs.writeFileSync('cpp/docs_gen/functions/'+fn+'_hit1.json', JSON.stringify(hits[1], null, 2));
console.log(JSON.stringify(hits[1], null, 2));
