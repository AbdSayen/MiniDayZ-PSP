const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

const hits = [];
function walk(evts, ctx) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes('SetMaxSpeed') || (s.includes('"Car"') && s.includes('31242'))) {
      /* noop */
    }
    // Car behavior SetMaxSpeed is act id 42 on behavior type - search numeric patterns
    if (/\[184,\s*42,|\[946,\s*42,|SetSpeed|MaxSpeed/.test(s)) hits.push(s.slice(0, 400));
    for (const x of e) if (Array.isArray(x)) walk(x, ctx);
  }
}

// Search for behavior acts: in C2, behavior actions are [objType, actId, "BehaviorName", ...]
// Car SetMaxSpeed = act 42 per c2runtime line 31242
function walk2(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (e.length >= 3 && e[2] === 'Car' && typeof e[1] === 'number') {
      const nums = [];
      (function cn(n) {
        if (typeof n === 'number' && n > 50 && n < 500) nums.push(n);
        else if (Array.isArray(n)) n.forEach(cn);
      })(e);
      if (e[1] === 42 || e[1] === 34) hits.push('Car act ' + e[1] + ' nums ' + nums.join(','));
    }
    for (const x of e) if (Array.isArray(x)) walk2(x);
  }
}

project[6].forEach((sh) => walk2(sh[1]));
console.log('car behavior acts', hits.length);
hits.slice(0, 40).forEach((h) => console.log(h));

// Dump t946 instance in layout for default behavior params
const layouts = project[5];
for (const lay of layouts) {
  const layers = lay[4];
  if (!layers) continue;
  for (const layer of layers) {
    const inst = layer[4];
    if (!inst) continue;
    for (const i of inst) {
      if (i[1] === 946 || i[1] === 806) {
        console.log('inst type', i[1], 'behaviors', JSON.stringify(i[7]).slice(0, 500));
      }
    }
  }
}
