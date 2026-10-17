const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8');
const json = JSON.parse(raw.slice(raw.indexOf('{')));
const game = json.project[6].find((s) => s[0] === 'Game_events');

function isGroup(e) {
  return e && e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true && typeof e[1][1] === 'string';
}
function findGroup(evts, name) {
  if (!Array.isArray(evts)) return null;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (isGroup(e) && e[1][1] === name) return e;
    for (const part of e) {
      if (Array.isArray(part) && part.length && Array.isArray(part[0])) {
        const f = findGroup(part, name);
        if (f) return f;
      }
    }
  }
  return null;
}
function foreachEvent(node, fn) {
  if (!Array.isArray(node)) return;
  if (node[0] === 0 && (node[1] === null || node[1] === false || typeof node[1] === 'number')) fn(node);
  if (isGroup(node)) {
    for (const part of node) {
      if (Array.isArray(part) && part.length && Array.isArray(part[0]))
        for (const child of part) foreachEvent(child, fn);
    }
  } else {
    for (const c of node) {
      if (Array.isArray(c) && c.length && Array.isArray(c[0])) for (const child of c) foreachEvent(child, fn);
      else if (Array.isArray(c)) foreachEvent(c, fn);
    }
  }
}

const mapGen = findGroup(game[1], 'Map generation');

function eventHasOnFn(e, name) {
  return new RegExp('180,40[^\\]]{0,120}"' + name + '"').test(JSON.stringify(e));
}

let createLoc = null;
foreachEvent(mapGen, (e) => {
  if (eventHasOnFn(e, 'create_map_location')) createLoc = e;
});

// Collect SetTileRange (185) patterns: tile id + width/height
const ranges = [];
const sets = {};
const waterSets = [];
const envSets = [];

function num(expr) {
  // unwrap simple [0, N] or [23,"var"]
  if (!Array.isArray(expr)) return expr;
  if (expr[0] === 0 && typeof expr[1] === 'number') return expr[1];
  if (expr[0] === 23) return 'var:' + expr[1];
  if (expr[0] === 4 && Array.isArray(expr[1])) {
    // add
    return 'add(' + num(expr[1][0]) + ',' + num(expr[1][1]) + ')';
  }
  if (expr[0] === 5) return 'sub(' + num(expr[1]) + ',' + num(expr[2]) + ')';
  if (expr[0] === 20) return 'obj' + expr[1] + '.e' + expr[2];
  return JSON.stringify(expr).slice(0, 80);
}

function walkActs(n) {
  if (!Array.isArray(n)) return;
  if (n[0] === 945 && n[1] === 185 && Array.isArray(n[n.length - 1])) {
    const p = n[n.length - 1];
    // params: tx,ty,tw,th,tile,state
    ranges.push({
      tx: num(p[0] && p[0][1]),
      ty: num(p[1] && p[1][1]),
      tw: num(p[2] && p[2][1]),
      th: num(p[3] && p[3][1]),
      tile: num(p[4] && p[4][1]),
    });
  }
  if (n[0] === 945 && n[1] === 188 && Array.isArray(n[n.length - 1])) {
    const p = n[n.length - 1];
    const tile = num(p[3] && p[3][1]);
    const k = String(tile);
    sets[k] = (sets[k] || 0) + 1;
  }
  if (n[0] === 945 && n[1] === 189 && Array.isArray(n[n.length - 1])) {
    const p = n[n.length - 1];
    const tile = num(p[2] && p[2][1]);
    const k = '189:' + tile;
    sets[k] = (sets[k] || 0) + 1;
  }
  if (n[0] === 199 && n[1] === 188) {
    const p = n[n.length - 1];
    waterSets.push(p && JSON.stringify(p).slice(0, 200));
  }
  if (n[0] === 221 && n[1] === 189) {
    const p = n[n.length - 1];
    envSets.push(p && JSON.stringify(p).slice(0, 200));
  }
  n.forEach(walkActs);
}
walkActs(createLoc);

console.log('SetTileRange count', ranges.length);
// summarize unique (tw,th,tile)
const rangeKeys = {};
for (const r of ranges) {
  const k = twth(r);
  rangeKeys[k] = (rangeKeys[k] || 0) + 1;
}
function twth(r) {
  return `tw=${r.tw} th=${r.th} tile=${r.tile} tx=${typeof r.tx === 'string' ? r.tx : r.tx} ty=${typeof r.ty === 'string' ? r.ty : r.ty}`;
}
console.log('Unique SetTileRange patterns (top 40):');
Object.entries(rangeKeys)
  .sort((a, b) => b[1] - a[1])
  .slice(0, 40)
  .forEach(([k, v]) => console.log(v, k));

console.log('\\nSetTile tile-id histogram (188/189):');
Object.entries(sets)
  .sort((a, b) => b[1] - a[1])
  .slice(0, 30)
  .forEach(([k, v]) => console.log(v, 'tile', k));

console.log('\\nWater SetTile samples', waterSets.length);
waterSets.slice(0, 5).forEach((s) => console.log(s));
console.log('\\nEnv SetTile samples', envSets.length);
envSets.slice(0, 5).forEach((s) => console.log(s));

// map_element instance var compares in create_map_location (507:71)
const ivCompares = {};
(function w(n) {
  if (!Array.isArray(n)) return;
  if (n[0] === 507 && n[1] === 71) {
    const p = n[n.length - 1];
    const s = JSON.stringify(p);
    ivCompares[s] = (ivCompares[s] || 0) + 1;
  }
  n.forEach(w);
})(createLoc);
console.log('\\nmap_element CompareIV top:');
Object.entries(ivCompares)
  .sort((a, b) => b[1] - a[1])
  .slice(0, 25)
  .forEach(([k, v]) => console.log(v, k.slice(0, 120)));

// What does create_map_element spawn? string literals that look like object types
let createEl = null;
foreachEvent(mapGen, (e) => {
  if (eventHasOnFn(e, 'create_map_element')) createEl = e;
});
const spawnStrs = new Set();
(function w(n) {
  if (typeof n === 'string' && /^[a-z0-9_]+$/i.test(n) && n.length > 2) spawnStrs.add(n);
  else if (Array.isArray(n)) n.forEach(w);
})(createEl);
console.log('\\ncreate_map_element strings:', [...spawnStrs].join(', '));

// Layout size constants used in mapgen
console.log('\\nworld_tilemaps dump filter check:');
const layouts = json.project[5];
const Map = layouts.find((L) => L[0] === 'Map');
console.log('Map', Map[1], 'x', Map[2]);

// Header of existing world_tilemaps.txt
const wt = fs.readFileSync(path.join(__dirname, '..', 'cpp', 'data', 'world_tilemaps.txt'), 'utf8');
const lines = wt.split(/\\n/);
console.log('world_tilemaps count line:', lines[0]);
let i = 1;
let n = parseInt(lines[0], 10);
for (let m = 0; m < n && i < lines.length; m++) {
  console.log(' map', lines[i].slice(0, 80));
  i += 2; // header + tiles line — but tiles may wrap? format is header\\n tiles\\n
}

// Actually format: count\\n then for each: header\\n tileids\\n
const parts = wt.trim().split('\\n');
console.log('total lines', parts.length);
for (let j = 1; j < parts.length; ) {
  console.log(parts[j]);
  j += 2;
}
