const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, pred, out) {
  if (!Array.isArray(evts)) return;
  const s = JSON.stringify(evts);
  if (pred(s)) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, pred, out);
}

const out = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s) =>
      (s.includes('"fuel"') || s.includes('bar_fuel') || s.includes('action_refuel')) &&
      (s.includes(',53,') || s.includes('"gas') || s.includes('Fueltank') || s.includes(',284')),
    out
  );
});
out.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log('matches', out.length);
for (let i = 0; i < Math.min(5, out.length); i++) {
  console.log('--- len', JSON.stringify(out[i]).length);
  console.log(JSON.stringify(out[i]).slice(0, 1500));
}
