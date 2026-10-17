const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8');
const json = JSON.parse(raw.slice(raw.indexOf('{')));
const events = json.project[6];
const rt = fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'c2runtime.js'), 'utf8');

// Map global ACE id -> name by scanning the big ace table array
const aceStart = rt.indexOf('cr.system_object.prototype.cnds.EveryTick');
console.log('ace table probe around EveryTick', aceStart);

// Better: find consecutive .acts. / .exps. / .cnds. entries and number them
const aceRe = /cr\.(?:plugins_|system_object|behaviors)\.[\w.]+/g;
const all = [];
let m;
while ((m = aceRe.exec(rt)) !== null) {
  // only the dense table near the end — skip early source definitions
  if (m.index < 300000) continue;
  all.push({ i: m.index, name: m[0] });
}
// Filter to unique sequential table (comma-separated list)
console.log('late ace refs', all.length);

const tilemapAces = all
  .map((a, idx) => ({ idx, name: a.name }))
  .filter((a) => a.name.includes('Tilemap'));
console.log('Tilemap ACEs in late table (index in late list):');
tilemapAces.forEach((a) => console.log(a.idx, a.name));

function findFnBodies(node, name, acc) {
  if (!Array.isArray(node)) return;
  // Function On: [180, 40, ..., "name"]
  let isOn = false;
  if (node[0] === 180 && node[1] === 40) {
    const flat = JSON.stringify(node);
    if (flat.includes('"' + name + '"')) isOn = true;
  }
  if (isOn) {
    // parent event includes actions — walk up via caller
  }
  for (const c of node) findFnBodies(c, name, acc);
}

// Collect full events that contain On function "name"
function walkEvents(list, name, acc) {
  if (!Array.isArray(list)) return;
  for (const e of list) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes('"' + name + '"') && s.includes(',40,') && s.length < 3000000) {
      // Prefer On function trigger
      if (s.indexOf('"' + name + '"') < 200 || s.includes('"generate_location"')) acc.push(e);
    }
    for (const part of e) {
      if (Array.isArray(part) && part.length && Array.isArray(part[0])) walkEvents(part, name, acc);
    }
  }
}

const tileTypes = new Set([199, 202, 221, 411, 475, 681, 682, 771, 779, 781, 945]);

function countTileOps(node, counts) {
  if (!Array.isArray(node)) return;
  if (typeof node[0] === 'number' && typeof node[1] === 'number' && tileTypes.has(node[0])) {
    const k = node[0] + ':' + node[1];
    counts[k] = (counts[k] || 0) + 1;
  }
  for (const c of node) countTileOps(c, counts);
}

function collectStrings(node, set) {
  if (typeof node === 'string' && node.length > 1 && node.length < 80) set.add(node);
  else if (Array.isArray(node)) node.forEach((x) => collectStrings(x, set));
}

for (const name of [
  'generate_location',
  'create_map_element',
  'create_map_location',
  'erase_map_element',
  'erase_map_location',
  'generate_mapgen_toelements',
  'generate_array_locations',
  'generate_array_locations_TD',
  'generate_array_roads_1',
  'generate_array_roads_2',
  'generate_array_tomap',
  'generate_helicrashes',
  'generate_bunker_entrance',
  'generate_minimap',
  'check_map_edges',
  'Reload_locations',
  'Reload_locations_start',
  'Reload_locations_aftersave',
]) {
  const acc = [];
  walkEvents(events, name, acc);
  acc.sort((a, b) => JSON.stringify(b).length - JSON.stringify(a).length);
  const body = acc[0];
  if (!body) {
    console.log('\\n##', name, 'NOT FOUND');
    continue;
  }
  const counts = {};
  countTileOps(body, counts);
  const strs = new Set();
  collectStrings(body, strs);
  const interesting = [...strs].filter(
    (x) =>
      !/^[0-9.]+$/.test(x) &&
      (x.includes('map') ||
        x.includes('Map') ||
        x.includes('Plant') ||
        x.includes('Tile') ||
        x.includes('hot') ||
        x.includes('river') ||
        x.includes('heli') ||
        x.includes('generate') ||
        x.includes('create') ||
        x.includes('erase') ||
        x.includes('Reload') ||
        x.includes('check') ||
        x.includes('Array') ||
        x.length < 30)
  );
  console.log('\\n##', name, 'hits', acc.length, 'bodyLen', JSON.stringify(body).length);
  console.log(
    'tileOps',
    Object.entries(counts)
      .sort((a, b) => b[1] - a[1])
      .slice(0, 15)
      .map(([k, v]) => k + '=' + v)
      .join(' ')
  );
  console.log('strs', interesting.slice(0, 40).join(' | '));
}

// Decode ACE ids 184-196 by finding their position in the global ace list
// C2 exports often store ace ids matching order in c2runtime's final list.
const tableMatch = rt.match(/cr\.system_object\.prototype\.cnds\.OnLayoutStart[\s\S]{0,500000}?cr\.plugins_\.Tilemap\.prototype\.acts\.SetTileRange/);
console.log('\\nfound SetTileRange context?', !!tableMatch);

// Build ordered list from the giant comma-separated assignment at end
const startMarker = 'cr.plugins_.Sprite.prototype.acts.Spawn';
const si = rt.lastIndexOf(startMarker);
console.log('Spawn marker', si);
// Instead parse every "cr.plugins_.Tilemap.prototype.(acts|exps|cnds).Name"
const tm = [...rt.matchAll(/cr\.plugins_\.Tilemap\.prototype\.(acts|exps|cnds)\.(\w+)/g)];
console.log('\\nAll Tilemap ACE mentions with char offset:');
tm.forEach((x, i) => {
  if (x.index > 300000) console.log(i, x.index, x[1] + '.' + x[2]);
});
