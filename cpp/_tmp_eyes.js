const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of [
  'zed_normal_eyes','zed_fast_eyes','zed_army_eyes','zed_screamer_eyes','zed_tank_eyes',
  'b_shadows','cloud_shadow','reloading_icon','reloading_spinner','rain_particle',
  'headshot_particles','particles'
]) {
  const m = r.match(new RegExp(n + '-sheet0\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)'));
  const m2 = r.match(new RegExp(n + '\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)'));
  console.log(n, m ? m.slice(1).join(',') : (m2 ? m2.slice(1).join(',') : 'MISS'));
}
