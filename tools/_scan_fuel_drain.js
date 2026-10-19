const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, pred, out, depth) {
  if (!Array.isArray(evts) || depth > 80) return;
  const s = JSON.stringify(evts);
  if (pred(s, evts)) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, pred, out, depth + 1);
}

const hits = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s) => s.includes('Drivable vehicles') && (s.includes('"fuel"') || s.includes(',284,') || s.includes('bar_fuel')),
    hits,
    0
  );
});
console.log('hits', hits.length);

// Scan all Game_events for Set IV 284 with arithmetic
const hits2 = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s, e) => {
      return (
        (s.includes('946') && s.includes('284')) ||
        s.includes('"bar_fuel"') ||
        (Array.isArray(e) && e[0] === 22 && e[1] === 946)
      );
    },
    hits2,
    0
  );
});
console.log('iv284 hits', hits2.length);
// Sample small events
hits2
  .map((h) => JSON.stringify(h).length)
  .sort((a, b) => a - b)
  .slice(0, 20)
  .forEach((l) => console.log(' len', l));
