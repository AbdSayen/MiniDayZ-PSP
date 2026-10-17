const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

function animsFor(skin) {
  const nameRe = /"([a-z0-9_]+)"/g;
  const near = [];
  let nm;
  while ((nm = nameRe.exec(raw))) {
    const snip = raw.slice(nm.index, nm.index + 1800);
    if (!snip.includes(skin + '-sheet')) continue;
    const fr = [];
    const fre = new RegExp('\\["images\\/(' + skin + '-sheet\\d+\\.png)",\\s*[^,]+,\\s*(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)', 'g');
    let fm;
    while ((fm = fre.exec(snip)) && fr.length < 5) {
      fr.push(fm[1].replace(skin+'-','') + ' ' + fm[2]+','+fm[3]+' '+fm[4]+'x'+fm[5]);
    }
    if (fr.length) near.push(nm[1] + ' => ' + fr.join(' | '));
  }
  return [...new Set(near)];
}

const want = /^(idle_|run_|pistol_|twohand_|axe_|using_|walk_|Default)/;
console.log('=== player ===');
for (const line of animsFor('player_skin_def_1')) {
  if (want.test(line)) console.log(line);
}
console.log('=== hands ===');
for (const line of animsFor('player_hands_white')) {
  if (want.test(line)) console.log(line);
}

// bunker tilemap size
const re = /\["images\/tilemap_bunker\.png",\s*[^,]+,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)/;
const m = raw.match(re);
console.log('bunker frame', m && m.slice(1));

// item icon examples
for (const n of ['item_ak','item_axe','item_bandage','item_can','gui_btn_inventory']) {
  const r = new RegExp('\\["images\\/(' + n + '[^"]*)\\.png",\\s*[^,]+,\\s*(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)');
  const mm = raw.match(r);
  console.log(n, mm && mm.slice(1));
}
