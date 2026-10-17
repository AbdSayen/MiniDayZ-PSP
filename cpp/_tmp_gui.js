const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of [
  'gui_btn_interact','gui_btn_attack','gui_btn_reload','gui_dpad_field','gui_dpad_stick',
  'gui_btn_inventory','gui_btn_zoom','spawn_loot_point','zed_bullet'
]) {
  const re = new RegExp(n + '-sheet0\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)');
  const m = raw.match(re);
  console.log(n, m ? m.slice(1).join(',') : 'MISS');
}
