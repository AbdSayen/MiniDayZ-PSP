const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of ['city3', 'city2', 'city', 'barrack', 'church', 'gas', 'firestation', 'police', 'hospital']) {
  const i = r.indexOf(`[2,"${n}"]`);
  console.log('\n---', n, i);
  if (i < 0) continue;
  console.log(r.slice(i - 150, i + 2500).replace(/\n/g, ' ').slice(0, 1800));
}
