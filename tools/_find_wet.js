const fs = require('fs');
const t = fs.readFileSync(require('path').join(__dirname, '../original_docs/data.js'), 'utf8');
for (const k of ['Wet', 'wet', 'Soaked', 'IV16', 'Rain_wet', 'player_wet', 'Humidity']) {
  let c = 0, i = 0;
  while ((i = t.indexOf(k, i)) >= 0 && c < 3) {
    console.log(k, '@', i, t.slice(i, i + 80).replace(/\s+/g, ' '));
    i++;
    c++;
  }
}
