const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
// C2 animation frames look like: ["images/player_skin_def_1-sheet0.png", false? or 0, x, y, w, h, ...
const re = /\["images\/(player_skin_def_1-sheet\d+\.png)",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/g;
const frames = [];
let m;
while ((m = re.exec(raw))) {
  frames.push({ img: m[1], x: +m[2], y: +m[3], w: +m[4], h: +m[5], at: m.index });
}
console.log('player frames', frames.length);
// unique by x,y
const uniq = new Map();
for (const f of frames) {
  const k = f.img + ':' + f.x + ',' + f.y;
  if (!uniq.has(k)) uniq.set(k, f);
}
console.log([...uniq.values()].slice(0, 40).map(f => `${f.img} ${f.x},${f.y} ${f.w}x${f.h}`).join('\n'));

// Find animation names near run_down for player
const names = [];
const nameRe = /"((?:run|walk|idle|Default|stopped)_[a-z]+|Default)"/g;
let nm;
const near = [];
while ((nm = nameRe.exec(raw))) {
  if (Math.abs(nm.index - (frames[0] ? frames[0].at : 0)) < 500000) {
    // check if player_skin appears within next 2000 chars
    const snip = raw.slice(nm.index, nm.index + 2500);
    if (snip.includes('player_skin_def_1-sheet')) {
      // extract first few frames in this snip
      const fr = [];
      const fre = /\["images\/(player_skin_def_1-sheet\d+\.png)",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/g;
      let fm;
      while ((fm = fre.exec(snip)) && fr.length < 6) {
        fr.push(`${fm[2]},${fm[3]} ${fm[4]}x${fm[5]}`);
      }
      if (fr.length) near.push(nm[1] + ' => ' + fr.join(' | '));
    }
  }
}
console.log('anims with player nearby:');
console.log([...new Set(near)].slice(0, 40).join('\n'));

// gui bar sizes
const gre = /\["images\/(gui_(?:blood|food|water|heat)_bar-sheet0\.png)",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/g;
while ((m = gre.exec(raw))) {
  console.log('gui', m[1], m[2], m[3], m[4], m[5]);
}
const gre2 = /\["images\/(gui_panel-sheet0\.png)",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/g;
while ((m = gre2.exec(raw))) {
  console.log('panel', m[2], m[3], m[4], m[5]);
  break;
}

// deer
const dre = /\["images\/(deer_skin-sheet0\.png)",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/g;
const deer = [];
while ((m = dre.exec(raw)) && deer.length < 20) {
  deer.push(`${m[2]},${m[3]} ${m[4]}x${m[5]}`);
}
console.log('deer frames', deer.join(' | '));
