const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of [
  'pistol_down','pistol_up','pistol_left','pistol_right',
  'twohand_down','twohand_up','twohand_left','twohand_right'
]) {
  let i = 0, c = 0;
  while (c < 2) {
    const a = r.indexOf('"' + n + '"', i);
    if (a < 0) break;
    const s = r.slice(a, a + 420);
    if (s.includes('player_skin_def_1')) {
      const m = s.match(/player_skin_def_1-sheet(\d+)\.png",\d+,(\d+),(\d+),(\d+),(\d+)/);
      console.log('body', n, m ? ('s'+m[1]+'('+m[2]+','+m[3]+')') : '?', s.slice(0, 200));
      c++;
    }
    i = a + 1;
  }
}
for (const n of [
  'pistol_down','pistol_up','pistol_left','pistol_right',
  'twohand_down','twohand_up','twohand_left','twohand_right'
]) {
  let i = 0;
  while (true) {
    const a = r.indexOf('"' + n + '"', i);
    if (a < 0) break;
    const s = r.slice(a, a + 420);
    if (s.includes('player_hands_white')) {
      const m = s.match(/player_hands_white-sheet(\d+)\.png",\d+,(\d+),(\d+)/);
      console.log('hands', n, m ? ('s'+m[1]+'('+m[2]+','+m[3]+')') : '?');
      break;
    }
    i = a + 1;
  }
}
