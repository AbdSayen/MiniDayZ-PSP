const fs = require('fs');
let s = fs.readFileSync('original_docs/data.js', 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);
for (const name of ['river_X', 'river_Y', 'river_R', 'River_ready', 'Bridges', 'Bridges_2']) {
  const read = `[23,"${name}"]`;
  let c = 0,
    p = 0;
  while ((p = s.indexOf(read, p)) >= 0) {
    c++;
    p++;
  }
  console.log('read', name, c);
}
