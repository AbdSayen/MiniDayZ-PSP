const fs = require('fs');
const t = fs.readFileSync(require('path').join(__dirname, '../original_docs/data.js'), 'utf8');
const keys = [
  '"Wet"', '"wet"', 'Soaked', 'Humidity', 'Rain_player', 'player_rain',
  'IV21', 'IV25', 'GetWet', 'MakeWet', 'drench', 'Drench', 'soak', 'Soak'
];
for (const k of keys) {
  const i = t.indexOf(k);
  console.log(k, i >= 0 ? 'FOUND ' + i : 'no');
}
// globals containing wet
const re = /"(?=[A-Za-z_]{0,20}[Ww]et)[A-Za-z_][A-Za-z0-9_]{2,40}"/g;
const s = new Set();
let m;
while ((m = re.exec(t))) s.add(m[0]);
console.log('wet-ish globals/strings', [...s].slice(0, 40).join(', '));
