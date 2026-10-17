const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

function framesOf(skin, anim) {
  let idx = 0;
  while (true) {
    const a = raw.indexOf('"' + anim + '"', idx);
    if (a < 0) return null;
    if (raw[a + anim.length + 2] !== ',') { idx = a + 1; continue; }
    const snip = raw.slice(a, a + 3500);
    if (!snip.includes(skin + '-sheet')) { idx = a + 1; continue; }
    const marker = '[["images/' + skin;
    const framesStart = snip.indexOf(marker);
    if (framesStart < 0) { idx = a + 1; continue; }
    let depth = 0, end = framesStart;
    for (; end < snip.length; end++) {
      const ch = snip[end];
      if (ch === '[') depth++;
      else if (ch === ']') {
        depth--;
        if (depth === 0) { end++; break; }
      }
    }
    const body = snip.slice(framesStart, end);
    const frames = [];
    const re = new RegExp(skin + '-sheet(\\d+)\\.png",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)', 'g');
    let m;
    while ((m = re.exec(body))) {
      frames.push({ sheet: +m[1], x: +m[2], y: +m[3], w: +m[4], h: +m[5] });
    }
    return frames;
  }
}

for (const skin of ['player_skin_def_1', 'player_hands_white']) {
  console.log('====', skin);
  for (const n of [
    'idle_down','idle_up','idle_right','idle_left',
    'run_down','run_up','run_right','run_left',
    'pistol_down','pistol_up','pistol_right','pistol_left',
    'twohand_down','twohand_up','twohand_right','twohand_left',
    'axe_down','axe_up','axe_right','axe_left'
  ]) {
    const f = framesOf(skin, n);
    console.log(n, f ? f.map(x => 's' + x.sheet + '(' + x.x + ',' + x.y + ')').join(' ') : 'MISS');
  }
}
