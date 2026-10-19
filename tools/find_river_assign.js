const fs = require('fs');
const path = require('path');
let s = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);

function findAll(needle, max = 40) {
  const out = [];
  let p = 0;
  while ((p = s.indexOf(needle, p)) >= 0 && out.length < max) {
    out.push(s.slice(p - 100, p + 180).replace(/\s+/g, ' '));
    p += needle.length;
  }
  return out;
}

console.log('=== [11,"river_X"] or set river ===');
for (const h of findAll('[11,"river_X"]', 25)) console.log(h);

console.log('\n=== river_Y sets ===');
for (const h of findAll('[11,"river_Y"]', 25)) console.log(h);

console.log('\n=== river_R sets ===');
for (const h of findAll('[11,"river_R"]', 25)) console.log(h);

console.log('\n=== River_ready sets ===');
for (const h of findAll('[11,"River_ready"]', 15)) console.log(h);

console.log('\n=== Bridges array push (203 action) ===');
for (const h of findAll('[11,"Bridges"]', 15)) console.log(h);

console.log('\n=== tile 4 water set (199 object) ===');
for (const h of findAll('"tilemap_water"', 8)) console.log(h.slice(0, 400));
