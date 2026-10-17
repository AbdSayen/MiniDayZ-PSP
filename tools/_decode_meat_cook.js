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

// Object type names: project[3] = object types [name, sid, ...]
const objTypes = project[3] || [];
const bySid = {};
for (const o of objTypes) {
  if (Array.isArray(o) && typeof o[0] === 'string') bySid[o[1]] = o[0];
}
console.log('obj253', bySid[253] || 'UNKNOWN');
console.log('obj252', bySid[252] || 'UNKNOWN');
console.log('obj172', bySid[172] || 'UNKNOWN');

// meat_cook occurrences with surrounding CompareParam ids
let idx = 0;
const cookCtx = [];
while ((idx = raw.indexOf('"meat_cook"', idx)) >= 0) {
  const slice = raw.slice(Math.max(0, idx - 800), idx + 400);
  const ids = [...slice.matchAll(/\[7,\[0,(\d+)\]\]/g)].map((m) => +m[1]);
  cookCtx.push({ idx, ids: [...new Set(ids)].filter((n) => n >= 20 && n <= 220).slice(0, 12) });
  idx += 11;
}
console.log('meat_cook PlayByName contexts', cookCtx.length);
cookCtx.forEach((c, i) => console.log(i, 'ids near', c.ids.join(',')));

// item_activate CompareParam for 210
const iaHits = [];
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (
      e[0] === 0 &&
      s.includes('"item_activate"') === false &&
      s.includes('[0,[0,0]],[8,0],[7,[0,210]]]') &&
      s.length < 8000 &&
      s.length > 200
    ) {
      iaHits.push(s.length);
    }
  })
);
console.log('CompareParam 210 branch lens', iaHits.slice(0, 20));

// Search pan / 210 in item_activate specifically
let iaRoot = null;
let iaLen = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"item_activate"]') && s.length > iaLen && s.length < 250000) {
      iaRoot = e;
      iaLen = s.length;
    }
  })
);
console.log('item_activate len', iaLen);
const iaStr = JSON.stringify(iaRoot);
console.log('item_activate has CompareParam 210', iaStr.includes('[0,[0,0]],[8,0],[7,[0,210]]]'));
console.log('item_activate has literal 210 count', (iaStr.match(/\[0,210\]/g) || []).length);
console.log('item_activate has Pan string', iaStr.includes('Pan'));
console.log('item_activate Spawn_drop count', (iaStr.match(/Spawn_drop/g) || []).length);
console.log('item_activate Check_space_inventory cook calls:');
for (const id of [60, 61, 71, 72, 91, 92, 93, 94, 95, 96]) {
  const re = new RegExp(
    '\\[0,\\[0,0\\]\\],\\[8,0\\],\\[7,\\[0,' + id + '\\]\\][\\s\\S]{0,2500}?Check_space_inventory[\\s\\S]{0,200}?\\[0,(\\d+)\\]'
  );
  const m = iaStr.match(re);
  // also look for [13,-1,XX]
  const re2 = new RegExp(
    '\\[0,\\[0,0\\]\\],\\[8,0\\],\\[7,\\[0,' + id + '\\]\\][\\s\\S]{0,3000}?\\[2,"Check_space_inventory"\\][\\s\\S]{0,120}?\\[0,-1\\][\\s\\S]{0,40}?\\[0,(\\d+)\\]'
  );
  const m2 = iaStr.match(re2);
  console.log('  id', id, 'csi->', m2 ? m2[1] : 'no -1,N', 'spawn_drop?', iaStr.includes('"Spawn_drop"') && id);
}

// Check_space_inventory with -1 meaning consume?
let csi = null;
let csiLen = 0;
project[6].forEach((sh) =>
  walk(sh[1], (e) => {
    const s = JSON.stringify(e);
    if (e[0] === 0 && s.includes('[2,"Check_space_inventory"]') && s.includes('OnFunction') && s.length > csiLen && s.length < 120000) {
      csi = e;
      csiLen = s.length;
    }
  })
);
console.log('Check_space_inventory OnFunction len', csiLen);
const csiStr = JSON.stringify(csi || []);
console.log('CSI Spawn_drop', (csiStr.match(/Spawn_drop/g) || []).length);
console.log('CSI has -1 param checks', csiStr.includes('[0,-1]') || csiStr.includes(',-1,'));

// Where does pan 210 appear outside item_activate?
const panRefs = [];
let p = 0;
while ((p = raw.indexOf('[7,[0,210]]', p)) >= 0 && panRefs.length < 30) {
  const slice = raw.slice(Math.max(0, p - 300), p + 200);
  const names = [...slice.matchAll(/\[2,"([^"]+)"\]/g)].map((m) => m[1]).slice(0, 8);
  panRefs.push(names.join('|'));
  p += 10;
}
console.log('CompareParam/IV 210 nearby strings:');
[...new Set(panRefs)].slice(0, 25).forEach((x) => console.log(' ', x));

// Also search IV15 == 210 (weapon?) vs item id 210
console.log('items_eng Pan id check');
const items = JSON.parse(fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/items_eng.json'), 'utf8'));
const pan = (Array.isArray(items) ? items : items.items || Object.values(items)).find(
  (it) => it && (it.id === 210 || it.name === 'Pan')
);
console.log(pan || 'not found as array');
if (!pan && items['210']) console.log(items['210']);
