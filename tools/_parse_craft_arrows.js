const fs = require('fs');
const path = require('path');
const body = JSON.parse(
  fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/functions/Craft arrows_body.json'), 'utf8')
);
const s = JSON.stringify(body);
console.log('len', s.length);
// Find Spawn_drop calls and CompareParam item-ish
const drops = [];
function walk(n, depth) {
  if (!Array.isArray(n) || depth > 40) return;
  if (n[1] === 69 && Array.isArray(n[5]) && n[5][0] && n[5][0][1] === 'Spawn_drop') {
    drops.push(JSON.stringify(n[5]).slice(0, 200));
  }
  if (typeof n[0] === 'string' && /Craft|Spawn|arrow|stick|knife/i.test(n[0])) {
    // skip
  }
  for (const x of n) walk(x, depth + 1);
}
walk(body, 0);
console.log('Spawn_drop sites', drops.length);
drops.slice(0, 15).forEach((d, i) => console.log(i, d));

// Search for literal strings
for (const key of ['Craft arrows', 'Spawn_drop', '51', '38', '42']) {
  console.log('includes', key, s.includes(key) || s.includes('"'+key+'"'));
}

// Pull CompareParam and SetVar near Craft arrows string
const idx = s.indexOf('Craft arrows');
console.log('Craft arrows idx', idx);
console.log(s.slice(Math.max(0, idx - 200), idx + 400));
