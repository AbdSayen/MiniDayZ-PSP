const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

// Find object type block for player_skin_def_1 by locating first animation list near the type name
const typeIdx = raw.indexOf('"player_skin_def_1"');
console.log('typeIdx', typeIdx);
// Animations often appear as ["idle_down", <nums...>, [ [frame],... ]]
// Frame: ["images/player_skin_def_1-sheetN.png", false|0, x, y, w, h, ...]

function extractAnims(skin) {
  const animNames = [
    'idle_down','idle_up','idle_right','idle_left',
    'run_down','run_up','run_right','run_left',
    'pistol_down','pistol_up','pistol_right','pistol_left',
    'twohand_down','twohand_up','twohand_right','twohand_left',
    'axe_down','axe_up','axe_right','axe_left','using_item'
  ];
  const out = {};
  for (const an of animNames) {
    // find `"an"` then soon after a frames array with this skin
    let searchFrom = 0;
    let best = null;
    while (searchFrom < raw.length) {
      const a = raw.indexOf('"' + an + '"', searchFrom);
      if (a < 0) break;
      const window = raw.slice(a, a + 4000);
      const frames = [];
      const fre = new RegExp('\\["images\\/(' + skin + '-sheet(\\d+)\\.png)",[^\\]]*?(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)', 'g');
      let fm;
      while ((fm = fre.exec(window))) {
        // stop if we somehow jumped too far - max 8 frames
        if (frames.length >= 8) break;
        frames.push({ sheet: +fm[2], x:+fm[3], y:+fm[4], w:+fm[5], h:+fm[6] });
      }
      // Prefer match that has at least 1 frame and isn't empty
      if (frames.length > 0) {
        // Heuristic: animation name should appear before frames; check distance to first frame image
        const firstImg = window.indexOf('images/' + skin);
        if (firstImg > 0 && firstImg < 3500) {
          best = frames;
          break;
        }
      }
      searchFrom = a + 1;
    }
    if (best) out[an] = best;
  }
  return out;
}

const anims = extractAnims('player_skin_def_1');
for (const [k,v] of Object.entries(anims)) {
  console.log(k + ': ' + v.map(f => `s${f.sheet}(${f.x},${f.y})`).join(' '));
}

// tilemap_bunker file
const path = require('path');
const imgdir = 'C:/Users/abdur/Desktop/MiniDayZ/original_docs/images';
for (const f of ['tilemap_bunker.png','tilemap_bunker_collision.png','ground_lvl1_tilemap.png']) {
  try {
    const st = fs.statSync(path.join(imgdir, f));
    console.log(f, 'exists', st.size);
  } catch(e) { console.log(f, 'miss'); }
}

// list item sprites
const items = fs.readdirSync(imgdir).filter(x => x.startsWith('i_') || x.startsWith('item') || x.includes('inv_')).slice(0, 40);
console.log('item-like', items.join(', '));
