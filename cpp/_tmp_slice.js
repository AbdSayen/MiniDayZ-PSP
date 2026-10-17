const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

// Find first occurrence of run_down near player_skin_def_1-sheet0
let idx = 0;
let found = 0;
while (found < 3) {
  const a = raw.indexOf('"run_down"', idx);
  if (a < 0) break;
  const snip = raw.slice(a, a + 1200);
  if (snip.includes('player_skin_def_1')) {
    console.log('==== run_down hit', found, 'at', a);
    console.log(snip.slice(0, 900));
    found++;
  }
  idx = a + 1;
}

idx = 0; found = 0;
while (found < 2) {
  const a = raw.indexOf('"run_left"', idx);
  if (a < 0) break;
  const snip = raw.slice(a, a + 1200);
  if (snip.includes('player_skin_def_1')) {
    console.log('==== run_left hit', found, 'at', a);
    console.log(snip.slice(0, 900));
    found++;
  }
  idx = a + 1;
}

idx = 0; found = 0;
while (found < 2) {
  const a = raw.indexOf('"pistol_down"', idx);
  if (a < 0) break;
  const snip = raw.slice(a, a + 900);
  if (snip.includes('player_skin_def_1')) {
    console.log('==== pistol_down hit', found);
    console.log(snip.slice(0, 700));
    found++;
  }
  idx = a + 1;
}
