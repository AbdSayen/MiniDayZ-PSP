const fs = require('fs');
const path = require('path');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(evts, fn, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (e[0] === 0 && JSON.stringify(e).includes('"' + fn + '"')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, fn, out);
  }
}

const fn = process.argv[2];
const hits = [];
project[6].forEach((sh) => walk(sh[1], fn, hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
const pick =
  hits.find((h) => {
    const l = JSON.stringify(h).length;
    return l > 200 && l < 80000;
  }) || hits[0];
const lines = [];
function decodeEvent(e, indent) {
  if (!Array.isArray(e)) return;
  if (e.length >= 2 && typeof e[0] === 'number' && typeof e[1] === 'number') {
    const strs = [];
    (function collect(n) {
      if (typeof n === 'string') strs.push(n);
      else if (Array.isArray(n)) n.forEach(collect);
    })(e);
    const nums = [];
    (function cn(n) {
      if (typeof n === 'number') nums.push(n);
      else if (Array.isArray(n)) n.forEach(cn);
    })(e);
    if (
      strs.length <= 12 ||
      strs.some((s) => /fuel|refuel|284|294|100|40|Vehicle|uaz|volga/i.test(s))
    ) {
      lines.push(
        indent +
          'p' +
          e[0] +
          ' a' +
          e[1] +
          ' | ' +
          strs.join(' | ') +
          ' | nums:' +
          nums.filter((n) => n > 0 && n < 5000).slice(0, 20).join(',')
      );
    }
  }
  for (const x of e) if (Array.isArray(x)) decodeEvent(x, indent);
}
lines.push('=== ' + fn + ' len ' + JSON.stringify(pick).length + ' ===');
decodeEvent(pick, '');
console.log(lines.join('\n'));
