const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '..', 'original_docs', 'data.js'), 'utf8');
const json = JSON.parse(raw.slice(raw.indexOf('{')));
const events = json.project[6];
const game = events.find((s) => s[0] === 'Game_events');

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

// Find On Function event by name inside a group tree
function findOnFunction(node, name) {
  if (!Array.isArray(node)) return null;
  // Event: conditions include [180,40,...,"name"]
  if (node[0] === 0 && node[1] === null) {
    const s = JSON.stringify(node);
    // tight match: Function plugin On with this exact name as param
    if (s.includes('[180,40,') && s.includes('"' + name + '"')) {
      // Ensure the name is the function trigger, not a Call
      const idx = s.indexOf('"'+name+'"');
      const window = s.slice(Math.max(0, idx - 80), idx + name.length + 20);
      if (window.includes('180,40') || window.includes('[180,40')) return node;
    }
  }
  for (const c of node) {
    const f = findOnFunction(c, name);
    if (f) return f;
  }
  // also search nested children arrays
  if (Array.isArray(node)) {
    for (const c of node) {
      if (Array.isArray(c)) {
        const f = findOnFunction(c, name);
        if (f) return f;
      }
    }
  }
  return null;
}

function findOnFunctionLoose(node, name, acc) {
  if (!Array.isArray(node)) return;
  if (typeof node[0] === 'number' || node[0] === 0 || node[0] === null) {
    // look for condition array starting with 180,40
  }
  // Detect: [180, 40, null, 2, ..., false, [[1,[2,"name"]]]]
  if (node[0] === 180 && node[1] === 40) {
    const s = JSON.stringify(node);
    if (s.includes('"' + name + '"') && s.length < 500) {
      acc.push({ cond: node, parentHint: true });
    }
  }
  for (const c of node) findOnFunctionLoose(c, name, acc);
}

function findEventWithOnFn(root, name) {
  const hits = [];
  function walk(node, parent) {
    if (!Array.isArray(node)) return;
    if (node[0] === 180 && node[1] === 40) {
      const s = JSON.stringify(node);
      if (s === JSON.stringify([180,40,null,2,false,false,false,node[7],false,[[1,[2,name]]]]) ||
          (s.includes('"'+name+'"') && s.length < 300 && !s.includes('Call'))) {
        hits.push(parent);
      }
    }
    for (const c of node) {
      if (Array.isArray(c)) walk(c, node[0] === 0 ? node : parent);
    }
  }
  walk(root, null);
  return hits;
}

const mapGen = findGroup(game[1], 'Map generation');
console.log('Map generation found', !!mapGen);

const fnNames = [
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
];

function eventHasOnFn(e, name) {
  if (!Array.isArray(e)) return false;
  // conditions are typically e[5] or similar — scan shallow
  const s = JSON.stringify(e);
  // On function condition is short; Call function action also has the name
  // Pattern for On: 180,40,... "name" early, and not only as Call (69)
  const onPat = new RegExp('180,40[^\\]]{0,120}"' + name + '"');
  return onPat.test(s);
}

function foreachEvent(node, fn) {
  if (!Array.isArray(node)) return;
  if (node[0] === 0 && (node[1] === null || node[1] === false || typeof node[1] === 'number')) {
    fn(node);
  }
  if (isGroup(node)) {
    for (const part of node) {
      if (Array.isArray(part) && part.length && Array.isArray(part[0])) {
        for (const child of part) foreachEvent(child, fn);
      }
    }
  } else {
    for (const c of node) {
      if (Array.isArray(c) && c.length && Array.isArray(c[0])) {
        // subevents
        for (const child of c) foreachEvent(child, fn);
      } else if (Array.isArray(c)) {
        foreachEvent(c, fn);
      }
    }
  }
}

for (const name of fnNames) {
  const matches = [];
  foreachEvent(mapGen, (e) => {
    if (eventHasOnFn(e, name)) matches.push(e);
  });
  // pick smallest matching event that looks like the On handler (not a huge parent)
  matches.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const e = matches[0];
  if (!e) {
    console.log('\\n##', name, 'NO MATCH');
    continue;
  }
  const s = JSON.stringify(e);
  const tileTypes = new Set([199, 202, 221, 411, 475, 681, 682, 771, 779, 781, 945, 507, 501, 511]);
  const counts = {};
  function walk(n) {
    if (!Array.isArray(n)) return;
    if (typeof n[0] === 'number' && typeof n[1] === 'number' && tileTypes.has(n[0])) {
      counts[n[0] + ':' + n[1]] = (counts[n[0] + ':' + n[1]] || 0) + 1;
    }
    n.forEach(walk);
  }
  walk(e);
  const strs = new Set();
  (function ws(n) {
    if (typeof n === 'string' && n.length > 1 && n.length < 60) strs.add(n);
    else if (Array.isArray(n)) n.forEach(ws);
  })(e);
  console.log('\\n##', name, 'size', s.length, 'matches', matches.length);
  console.log(
    'ops',
    Object.entries(counts)
      .sort((a, b) => b[1] - a[1])
      .slice(0, 20)
      .map(([k, v]) => k + '=' + v)
      .join(' ')
  );
  console.log('strs', [...strs].slice(0, 35).join(' | '));
}

// Sample a few 945:188 actions to see parameters
console.log('\\n=== Sample 945 actions ===');
const samples = { 188: [], 189: [], 185: [], 184: [], 190: [], 196: [], 191: [], 194: [] };
foreachEvent(mapGen, (e) => {
  (function walk(n) {
    if (!Array.isArray(n)) return;
    if (n[0] === 945 && samples[n[1]] && samples[n[1]].length < 2) {
      samples[n[1]].push(JSON.stringify(n).slice(0, 500));
    }
    if ((n[0] === 199 || n[0] === 202 || n[0] === 411 || n[0] === 221) && samples[n[1]] && samples[n[1]].length < 2) {
      samples[n[1]].push('t' + n[0] + ':' + JSON.stringify(n).slice(0, 400));
    }
    n.forEach(walk);
  })(e);
});
for (const [k, v] of Object.entries(samples)) {
  console.log('ACE', k, v.join('\\n---\\n'));
}
