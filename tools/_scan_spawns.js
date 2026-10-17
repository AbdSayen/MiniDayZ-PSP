const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const analysis = JSON.parse(
  fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/c2_analysis.json'), 'utf8')
);
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

function extractFunctionBody(name) {
  const needle = '[1,[2,"' + name + '"]]';
  const start = raw.indexOf(needle);
  if (start < 0) return null;
  // Walk backward to event block start `[0,null,false,null,` after previous function
  let s = start;
  for (let i = 0; i < 5; i++) {
    const p = raw.lastIndexOf('[0,null,false,null,', s - 1);
    if (p >= 0 && start - p < 500) s = p;
  }
  // Heuristic end: next function definition at same indent level
  const endNeedle = '[1,[2,"';
  let end = raw.indexOf(endNeedle, start + needle.length + 100);
  // skip self references
  while (end > 0 && end - start < 5000000) {
    const snippet = raw.slice(end, end + 80);
    if (snippet.includes(name)) {
      end = raw.indexOf(endNeedle, end + 10);
      continue;
    }
    break;
  }
  if (end < 0) end = start + 3_000_000;
  return raw.slice(s, Math.min(end, s + 4_000_000));
}

for (const fn of ['create_map_element', 'generate_location', 'create_map_location']) {
  const body = extractFunctionBody(fn);
  console.log('\n====', fn, 'body len', body ? body.length : 0);
  if (!body) continue;

  // Spawn by object type index: patterns like [0,321] after Create action markers
  const spawnByIdx = {};
  for (const m of body.matchAll(/\[0,(\d{2,3})\]/g)) {
    const idx = +m[1];
    if (idxToPrefix[idx] && idx >= 311 && idx <= 395) {
      spawnByIdx[idx] = (spawnByIdx[idx] || 0) + 1;
    }
  }
  const sorted = Object.entries(spawnByIdx).sort((a, b) => b[1] - a[1]);
  console.log('Object idx hits (311-395):');
  for (const [idx, c] of sorted) console.log(' ', idx, idxToPrefix[idx], c);

  // Offset literals: pairs of [0,x] where x is multiple of 50 or common offsets
  // Look for Create at x,y: [0,[4,[20,507,65... offset patterns [19,95,[[0,N]
  const offsetFreq = {};
  for (const m of body.matchAll(/\[19,95,\[\[0,(-?\d+)\],\[0,(-?\d+)\]\]\]/g)) {
    const key = m[1] + ',' + m[2];
    offsetFreq[key] = (offsetFreq[key] || 0) + 1;
  }
  const topOff = Object.entries(offsetFreq)
    .sort((a, b) => b[1] - a[1])
    .slice(0, 40);
  console.log('Top random offset pairs [19,95]:', topOff.map(([k, v]) => k + '=' + v).join('; '));
}

// Search create_map_element for compare on param 151 (location type id?)
const body = extractFunctionBody('create_map_element');
if (body) {
  const cmp = {};
  for (const m of body.matchAll(/\[7,\[0,(\d+)\]\]/g)) {
    const n = +m[1];
    if (n >= 0 && n <= 20) cmp[n] = (cmp[n] || 0) + 1;
  }
  console.log('\ncreate_map_element [7,[0,N]] small N freq:', cmp);
}
