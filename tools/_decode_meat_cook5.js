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

// Dump Check_space_backpack OnFunction focusing on -1
let bp = null;
let bpL = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.slice(0, 200).includes('[2,"Check_space_backpack"]')) return;
    if (!s.includes(',40,')) return;
    if (s.length > bpL && s.length < 20000) {
      bp = e;
      bpL = s.length;
    }
  })
);
console.log('Check_space_backpack len', bpL);
function dump(e, d) {
  if (!Array.isArray(e) || d > 14) return;
  if (typeof e[0] === 'number' && typeof e[1] === 'number') {
    const s = JSON.stringify(e);
    const strs = [...s.matchAll(/\[2,"([^"]+)"\]/g)].map((m) => m[1]);
    const nums = [...s.matchAll(/\[0,(-?\d+)\]/g)].map((m) => m[1]).slice(0, 16);
    if (strs.length || nums.some((n) => +n === -1 || (+n > 0 && +n < 300)))
      console.log('  '.repeat(Math.min(d, 8)) + `p${e[0]}a${e[1]} ${strs.join(',')} nums:${nums.join(',')}`);
  }
  for (const x of e) if (Array.isArray(x)) dump(x, d + 1);
}
dump(bp, 0);

// Dump Check_space_inventory fully
let inv = null;
let invL = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.slice(0, 200).includes('[2,"Check_space_inventory"]')) return;
    if (!s.includes(',40,')) return;
    if (s.length > invL && s.length < 10000) {
      inv = e;
      invL = s.length;
    }
  })
);
console.log('\n=== Check_space_inventory full dump len', invL, '===');
dump(inv, 0);

// Parent group: find event that contains BOTH OnFunction Check_space_inventory AND CompareParam -1 as siblings
let group = null;
let gL = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('Picking inventory items')) return;
    if (s.length > gL && s.length < 30000) {
      group = e;
      gL = s.length;
    }
  })
);
console.log('\nPicking inventory items group len', gL);
// List top-level subevent signatures
if (group) {
  const kids = group[group.length - 1] || group[5] || [];
  // C2 event: [0, null/true, ..., conditions, actions, subevents]
  function listTop(e, depth) {
    if (!Array.isArray(e) || depth > 3) return;
    if (e[0] === 0 || e[0] === 1 || Array.isArray(e[0])) {
      const s = JSON.stringify(e);
      if (e[0] === 0 && s.length > 100) {
        const strs = [...new Set([...s.matchAll(/\[2,"([^"]{1,40})"\]/g)].map((m) => m[1]))].slice(0, 8);
        const hasNeg = s.includes('[7,[0,-1]]');
        if (strs.length || hasNeg)
          console.log(' sub len', s.length, 'neg1', hasNeg, strs.join('|'));
      }
    }
    // subevents usually last element
    const last = e[e.length - 1];
    if (Array.isArray(last) && depth < 2) {
      for (const x of last) if (Array.isArray(x)) listTop(x, depth + 1);
    }
  }
  listTop(group, 0);
}

// Is there OnFunction Check_space_inventory with -1 as SEPARATE event (not in 1250 body)?
const csiAll = [];
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    if (e[0] !== 0) return;
    const s = JSON.stringify(e);
    if (!s.includes('[2,"Check_space_inventory"]')) return;
    if (!s.includes(',40,')) return;
    csiAll.push(s.length);
  })
);
console.log('all Check_space_inventory OnFunction-ish lens', csiAll.sort((a, b) => a - b));

// Pan: melee NPC_get_damage CompareInstanceVar IV15==210
console.log('\nPan weapon: weapons.cpp already has melee. item_activate CompareParam 210?', false);
// Confirm no cook needs pan IV or overlapping pan object
let ia = null;
let iaL = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"item_activate"]') && s.length > iaL && s.length < 250000) {
      ia = e;
      iaL = s.length;
    }
  })
);
const cookSlice = JSON.stringify(ia).match(/meat_cook[\s\S]{0,50}/g);
console.log('meat_cook local', cookSlice);

// IV44 meaning near cook
const iv44 = [...JSON.stringify(ia).matchAll(/IV44[\s\S]{0,80}/g)].slice(0, 3);
console.log('IV44 samples', iv44.map((m) => m[0]));
