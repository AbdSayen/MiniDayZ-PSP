const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of [
  'muzzle_flash','test_bullet_hit','test_bodyhit','zed_normal_skin1_dead','bandit_dead',
  'deer_dead','berry_bush','beartrap_deployed','barbedwire','landmine','fire_place_warm',
  'car_uaz_base','wolf_skin_dead','dog_dead'
]) {
  let m = r.match(new RegExp(n + '-sheet0\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)'));
  if (!m) m = r.match(new RegExp(n + '\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)'));
  console.log(n, m ? m.slice(1).join(',') : 'MISS');
}
