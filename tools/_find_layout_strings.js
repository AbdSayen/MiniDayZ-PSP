const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const names = ['city','city2','city3','barrack','church','gas','firestation','firestation2','police','hospital','village','village1','village2','hostel','supermarket','school','helicrash'];
for (const name of names) {
  const needle = `[2,"${name}"]`;
  let pos = 0;
  const hits = [];
  while (true) {
    const i = raw.indexOf(needle, pos);
    if (i < 0) break;
    hits.push(i);
    pos = i + 1;
    if (hits.length >= 5) break;
  }
  if (hits.length) console.log(name, hits);
}
