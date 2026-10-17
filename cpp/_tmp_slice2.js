const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

function framesOf(skin, anim) {
  let idx = 0;
  while (true) {
    const a = raw.indexOf('"' + anim + '"', idx);
    if (a < 0) return null;
    const snip = raw.slice(a, a + 2500);
    if (!snip.includes(skin)) { idx = a + 1; continue; }
    // stop at next anim sibling `]],["next"`
    const end = snip.indexOf(']],["');
    const body = end > 0 ? snip.slice(0, end + 2) : snip;
    const frames = [];
    const re = new RegExp(skin + '-sheet(\\d+)\\.png",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)', 'g');
    let m;
    while ((m = re.exec(body))) {
      frames.push({ sheet:+m[1], x:+m[2], y:+m[3], w:+m[4], h:+m[5] });
    }
    return frames;
  }
}

const skin = 'player_skin_def_1';
const names = [
  'idle_down','idle_up','idle_right','idle_left',
  'run_down','run_up','run_right','run_left',
  'pistol_down','pistol_up','pistol_right','pistol_left',
  'twohand_down','twohand_up','twohand_right','twohand_left',
  'axe_down','axe_up','axe_right','axe_left'
];
for (const n of names) {
  const f = framesOf(skin, n);
  console.log(n, f ? f.map(x => `s${x.sheet}(${x.x},${x.y})`).join(' ') : 'MISS');
}

console.log('--- hands ---');
const hands = 'player_hands_white';
for (const n of names) {
  const f = framesOf(hands, n);
  if (f) console.log(n, f.map(x => `s${x.sheet}(${x.x},${x.y})`).join(' '));
}

// bunker tile props
const bi = raw.indexOf('tilemap_bunker');
console.log('bunker idx', bi);
console.log(raw.slice(bi, bi+400));
