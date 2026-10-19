const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  const s = JSON.stringify(evts);
  if (
    s.includes('Vehicle collisions') ||
    (s.includes('car_crash') && s.includes('946') && s.includes('294'))
  ) {
    if (s.length < 3000 && s.includes('[10,')) out.push(evts);
  }
  for (const e of evts) if (Array.isArray(e)) walk(e, out);
}

const out = [];
project[6].forEach((sh) => walk(sh[1], out));
out.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log('samples', out.length);
for (let i = 0; i < Math.min(15, out.length); i++) {
  const j = JSON.stringify(out[i]);
  if (j.includes('294') || j.includes('284'))
    console.log(j.slice(0, 600));
}
