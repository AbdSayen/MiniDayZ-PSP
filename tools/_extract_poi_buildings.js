// One-off: extract building refs / offsets from create_map_element in data.js
const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');

const fn = 'create_map_element';
const idx = raw.indexOf('"' + fn + '"');
console.log(fn, 'at byte', idx);
if (idx < 0) process.exit(1);

// Event sheet functions are huge; take 2MB forward from first ref (includes full function).
const win = raw.slice(idx, idx + 2_000_000);

const buildings = [
  'b_village_brown', 'b_village_green', 'b_village_red', 'b_village_yellow', 'b_village_yard',
  'b_village_red2', 'b_city_house_1', 'b_city_house_2', 'b_city_house_3', 'b_city_house_4',
  'b_hostel', 'b_supermarket', 'b_military_barrack', 'b_military_barrack2', 'b_military_shtab',
  'b_militay_tent', 'b_hospital', 'b_gas_station', 'b_police_station', 'b_fire_station',
  'b_church', 'b_garage_brown', 'b_garage_blue', 'b_shed', 'b_school', 'b_piano_house',
  'b_hunter_shelter', 'heli_crash', 'tree_pine', 'obst_fence', 'obst_block',
];
console.log('\nBuilding string hits in window:');
for (const b of buildings) {
  const c = win.split(b).length - 1;
  if (c > 0) console.log(' ', b, c);
}

// Location type strings passed to create_map_element (C2 uses short type names)
const typeRe = /"(village|city|military|gas|hospital|fire|police|church|heli|bunker|secret|hostel|school|supermarket)"/g;
const typeFreq = {};
let m;
while ((m = typeRe.exec(win)) !== null) {
  typeFreq[m[1]] = (typeFreq[m[1]] || 0) + 1;
}
console.log('\nType string literals:', typeFreq);

// Also scan generate_location
const gl = raw.indexOf('"generate_location"');
console.log('\ngenerate_location at', gl);
const win2 = raw.slice(gl, gl + 1_500_000);
for (const b of buildings) {
  const c = win2.split(b).length - 1;
  if (c > 0) console.log(' gen_loc', b, c);
}

// Object type indices in C2 data: search t311 style near Spawn object actions
// Pattern: object type index as [0, N] after create - look for clusters 311-333
const objIdx = {};
for (const mm of win.matchAll(/\[0,(\d{3})\]/g)) {
  const n = +mm[1];
  if (n >= 300 && n <= 360) objIdx[n] = (objIdx[n] || 0) + 1;
}
console.log('\nObject index literals 300-360 in create_map_element:', objIdx);
