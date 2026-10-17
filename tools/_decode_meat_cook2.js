const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;

function walk(evts, fn) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    fn(e);
    for (const x of e) walk(x, fn);
  }
}

// Resolve object names from project[3]
const types = project[3];
console.log('project[3] sample', JSON.stringify(types && types[0]).slice(0, 200));
console.log('project[3][1]', JSON.stringify(types && types[1]).slice(0, 200));
// Often: [null, [...objects]] or list of [name, ...]
function findTypeName(uid) {
  const stack = [project];
  while (stack.length) {
    const n = stack.pop();
    if (!Array.isArray(n)) continue;
    // C2 object type: ["Name", sid, ...]
    if (typeof n[0] === 'string' && n[1] === uid && n.length >= 3) return n[0];
    for (const x of n) if (Array.isArray(x)) stack.push(x);
  }
  return null;
}
for (const id of [172, 253, 252, 275, 664, 178, 180]) {
  console.log('type', id, findTypeName(id));
}

// Extract CompareParam 210 branches with readable strings
const branches210 = [];
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('[0,[0,0]],[8,0],[7,[0,210]]]') && !s.includes('[8,0],[7,[0,210]]')) return;
    if (s.length < 150 || s.length > 12000) return;
    const strs = [...s.matchAll(/\[2,"([^"]{1,80})"\]/g)].map((m) => m[1]);
    const uniq = [...new Set(strs)];
    branches210.push({ len: s.length, strs: uniq.slice(0, 25) });
  })
);
branches210.sort((a, b) => b.len - a.len);
console.log('--- branches involving literal 210 (top) ---');
const seen = new Set();
for (const b of branches210) {
  const key = b.strs.join(',');
  if (seen.has(key)) continue;
  seen.add(key);
  console.log('len', b.len, 'strs:', b.strs.join(' | '));
  if (seen.size >= 20) break;
}

// Check_space_inventory function body — find OnFunction
const csiHits = [];
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('[2,"Check_space_inventory"]')) return;
    if (!s.includes('OnFunction') && !s.includes(',40,')) return;
    // OnFunction is action type 40 on fn object
    if (s.length > 500 && s.length < 150000) {
      const strs = [...s.matchAll(/\[2,"([^"]{1,60})"\]/g)].map((m) => m[1]);
      csiHits.push({ len: s.length, strs: [...new Set(strs)].slice(0, 30), head: s.slice(0, 300) });
    }
  })
);
csiHits.sort((a, b) => a.len - b.len);
console.log('--- Check_space_inventory events ---');
csiHits.slice(0, 8).forEach((h, i) => {
  console.log(i, 'len', h.len, 'strs', h.strs.join(' | '));
});

// Decode cook branch raw JSON for id 60 to confirm Check_space_inventory args
let ia = null;
let iaLen = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"item_activate"]') && s.length > iaLen && s.length < 250000) {
      ia = e;
      iaLen = s.length;
    }
  })
);

function extractCook(id) {
  let found = null;
  walk([ia], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (s.includes('[0,[0,0]],[8,0],[7,[0,' + id + ']]]') && s.includes('meat_cook') && s.length < 3000) {
      found = e;
    }
  });
  return found;
}

const b60 = extractCook(60);
console.log('--- cook branch 60 compact ---');
function compact(e, depth) {
  if (!Array.isArray(e) || depth > 12) return;
  const s = JSON.stringify(e);
  if (e.length >= 2 && typeof e[0] === 'number' && typeof e[1] === 'number') {
    const strs = [...s.matchAll(/\[2,"([^"]+)"\]/g)].map((m) => m[1]);
    const nums = [...s.matchAll(/\[0,(-?\d+(?:\.\d+)?)\]/g)].map((m) => m[1]).slice(0, 15);
    if (strs.length || nums.some((n) => Math.abs(+n) <= 500 && +n !== 0)) {
      console.log(
        '  '.repeat(depth) +
          'p' +
          e[0] +
          'a' +
          e[1] +
          ' ' +
          strs.join(',') +
          ' nums:' +
          nums.join(',')
      );
    }
  }
  for (const x of e) if (Array.isArray(x)) compact(x, depth + 1);
}
compact(b60, 0);

// Does Check_space_inventory(-1, id) call Spawn_drop?
// Search for CallFunction Check_space_inventory with -1 near Spawn_drop definition
const csiBody = csiHits.find((h) => h.strs.includes('Check_space_inventory') && h.len > 2000);
if (csiBody) {
  // reload full
  let full = null;
  project[6].forEach((sh) =>
    walk(sh[1], (e) => {
      const s = JSON.stringify(e);
      if (e[0] === 0 && s.length === csiBody.len && s.includes('[2,"Check_space_inventory"]')) full = e;
    })
  );
  if (!full) {
    // find longest that starts with OnFunction Check_space_inventory
    let best = null;
    let bestL = 0;
    project[6].forEach((sh) =>
      walk(sh[1], (e) => {
        const s = JSON.stringify(e);
        if (
          e[0] === 0 &&
          s.includes('[2,"Check_space_inventory"]') &&
          /OnFunction|"Check_space_inventory"/.test(s) &&
          s.slice(0, 120).includes('Check_space_inventory') &&
          s.length > bestL &&
          s.length < 200000
        ) {
          best = e;
          bestL = s.length;
        }
      })
    );
    full = best;
    console.log('CSI best len', bestL);
  }
  const fs2 = JSON.stringify(full || []);
  console.log('CSI Spawn_drop', (fs2.match(/Spawn_drop/g) || []).length);
  console.log('CSI item_for_remove', (fs2.match(/item_for_remove/g) || []).length);
  console.log('CSI Destroy', (fs2.match(/Destroy/g) || []).length);
  console.log('CSI CompareParam -1', fs2.includes('[7,[0,-1]]') || fs2.includes('[0,-1]'));
  // show -1 related snippets
  let i = 0;
  let pos = 0;
  while ((pos = fs2.indexOf('[0,-1]', pos)) >= 0 && i < 5) {
    console.log('CSI -1 ctx', fs2.slice(pos - 80, pos + 120));
    pos += 6;
    i++;
  }
}

// Campfire timer Burn / IV0 lit
console.log('fireplace_loop count', (raw.match(/fireplace_loop/g) || []).length);
console.log('Burn timer tag count', (raw.match(/\[2,"Burn"\]/g) || []).length);
