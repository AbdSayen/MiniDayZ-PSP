const fs = require('fs');
const data = JSON.parse(fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8'));

// Walk project types looking for sprite animations
function walk(o, path, hits) {
  if (!o || typeof o !== 'object') return;
  if (Array.isArray(o)) {
    // Animation-like: [name, speed, loop?, ping?, ..., frames[]]
    if (typeof o[0] === 'string' && Array.isArray(o[o.length - 1]) && o[o.length - 1].length &&
        Array.isArray(o[o.length - 1][0]) && typeof o[o.length - 1][0][0] === 'string' &&
        String(o[o.length - 1][0][0]).includes('-sheet')) {
      hits.push({ path, name: o[0], frames: o[o.length - 1].slice(0, 6).map(f => ({
        img: f[0], x: f[2], y: f[3], w: f[4], h: f[5]
      }))});
    }
    for (let i = 0; i < o.length; i++) walk(o[i], path + '[' + i + ']', hits);
  } else {
    for (const k of Object.keys(o)) walk(o[k], path + '.' + k, hits);
  }
}

const hits = [];
walk(data, 'root', hits);
const want = ['run_down','run_up','run_right','run_left','Default','stopped','idle_down'];
const forSkin = hits.filter(h => h.frames.some(f => (f.img||'').includes('player_skin_def_1')));
console.log('player anims', forSkin.map(h => h.name));
for (const h of forSkin) {
  if (!want.includes(h.name) && !h.name.startsWith('run_') && !h.name.startsWith('idle')) continue;
  console.log(h.name, JSON.stringify(h.frames));
}

const gui = hits.filter(h => h.frames.some(f => (f.img||'').includes('gui_blood') || (f.img||'').includes('gui_panel')));
console.log('gui anims sample', gui.slice(0,5).map(h => [h.name, h.frames[0]]));

// Also dump instance positions for gui objects if we can find layout "GUI"
const s = JSON.stringify(data);
const idx = s.indexOf('"gui_blood_bar"');
console.log('gui_blood_bar idx', idx);
