const fs = require('fs');
const path = require('path');
const project = JSON.parse(fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')).project;

function walk(evts, fn, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('"' + fn + '"')) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, fn, out);
  }
}

const fn = process.argv[2] || 'Car_respawn_uaz';
const hits = [];
project[6].forEach(sh => walk(sh[1], fn, hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
// pick ~3330 len hit
const pick = hits.find(h => JSON.stringify(h).length > 2000 && JSON.stringify(h).length < 5000) || hits[0];
const outPath = path.join('cpp/docs_gen/functions', fn + '_decoded.txt');
const lines = [];
function w(s) { lines.push(s); }

function decodeEvent(e, indent) {
  if (!Array.isArray(e)) return;
  // action: [pluginId, actionId, ...]
  if (e.length >= 2 && typeof e[0] === 'number' && typeof e[1] === 'number' && e[0] > 0 && e[0] < 300) {
    const pid = e[0], aid = e[1];
    const strs = [];
    (function collect(n) {
      if (typeof n === 'string') strs.push(n);
      else if (Array.isArray(n)) n.forEach(collect);
    })(e);
    if (pid === 180 && aid === 69) w(indent + 'CallFunction: ' + strs.filter(s => s !== 'Function').join(', '));
    else if (pid === 4 || (pid === 7 && strs.some(x => x.startsWith('car_')))) {
      w(indent + 'Action p' + pid + ' a' + aid + ' strs: ' + strs.slice(0, 12).join(' | '));
    } else if (strs.some(s => /fuel|hp|speed|condition|trunk|Create|Spawn|Set|Pin|random|806|812/i.test(s) || strs.length <= 8)) {
      const nums = [];
      (function cn(n) { if (typeof n === 'number') nums.push(n); else if (Array.isArray(n)) n.forEach(cn); })(e);
      w(indent + 'Action p' + pid + ' a' + aid + ' | ' + strs.join(' | ') + ' | nums:' + nums.filter(n => n > 0 && n < 5000).slice(0, 15).join(','));
    }
  }
  for (const x of e) if (Array.isArray(x)) decodeEvent(x, indent);
}

w('=== ' + fn + ' len ' + JSON.stringify(pick).length + ' ===');
decodeEvent(pick, '');
fs.writeFileSync(outPath, lines.join('\n'));
console.log('wrote', outPath, 'lines', lines.length);

// Also decode Vehicle collisions group snippet for speed
for (const fn2 of ['Spawn_bandit_camp']) {
  const h2 = [];
  project[6].forEach(sh => walk(sh[1], fn2, h2));
  h2.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const p2 = h2.find(h => JSON.stringify(h).length > 800 && JSON.stringify(h).length < 2000) || h2[1] || h2[0];
  if (!p2) continue;
  const lines2 = [];
  function w2(s) { lines2.push(s); }
  w2('=== ' + fn2 + ' len ' + JSON.stringify(p2).length + ' ===');
  decodeEvent(p2, '');
  fs.writeFileSync(path.join('cpp/docs_gen/functions', fn2 + '_decoded.txt'), lines2.join('\n'));
  console.log('wrote', fn2, 'lines', lines2.length);
}
