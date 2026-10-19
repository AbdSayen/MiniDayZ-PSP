// Extract SetTile / SetTileRange patterns from create_map_location in data.js
const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const idx = raw.indexOf('"create_map_location"');
if (idx < 0) {
  console.error('create_map_location not found');
  process.exit(1);
}
// Take a large window forward (function body is huge)
const win = raw.slice(idx, idx + 900000);
console.log('window bytes', win.length);

// C2 action arrays often look like: [obj, actionId, null, uid, false, [[params]]]
// SetTile typically has params including tile id. Look for sequences with Tilemapspot
// and nearby numeric pairs.

const tileCounts = {};
for (const m of win.matchAll(/\[0,(\d+)\]/g)) {
  const t = m[1];
  tileCounts[t] = (tileCounts[t] || 0) + 1;
}
const topTiles = Object.entries(tileCounts)
  .map(([k, v]) => [+k, v])
  .sort((a, b) => b[1] - a[1])
  .slice(0, 30);
console.log('top literal ints in create_map_location window:');
console.log(topTiles.map(([k, v]) => k + ':' + v).join(', '));

// Ranges: consecutive [0,a],[0,b] where both small
const rangeFreq = {};
for (const m of win.matchAll(/\[0,(\d+)\],\[0,(\d+)\]/g)) {
  const a = +m[1], b = +m[2];
  if (a >= 1 && a <= 40 && b >= 1 && b <= 40) {
    const key = a + 'x' + b;
    rangeFreq[key] = (rangeFreq[key] || 0) + 1;
  }
}
console.log('common AxB ranges:');
console.log(
  Object.entries(rangeFreq)
    .sort((a, b) => b[1] - a[1])
    .slice(0, 40)
    .map(([k, v]) => k + '=' + v)
    .join(', ')
);

// Also dump location-type strings nearby create_map_element
const types = [...win.matchAll(/"(church|city|village|barrack|hospital|fire|gas|police|heli|bunker|secret|hunter|hostel)[^"]*"/gi)]
  .map(m => m[0]);
const typeFreq = {};
for (const t of types) typeFreq[t] = (typeFreq[t] || 0) + 1;
console.log('type strings:', Object.entries(typeFreq).sort((a, b) => b[1] - a[1]).slice(0, 40));
