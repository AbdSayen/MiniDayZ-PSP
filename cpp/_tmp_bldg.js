const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const names = [
  'b_hospital','b_gas_station','b_police_station','b_fire_station','b_church',
  'b_military_barrack','b_military_shtab','b_hunter_shelter','b_hostel',
  'b_village_brown','b_village_green','b_village_red','b_village_yellow','car_explode'
];
for (const n of names) {
  const re = new RegExp('\\["images\\/' + n + '-sheet0\\.png",\\s*[^,]+,\\s*(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)');
  const m = raw.match(re);
  if (m) console.log(n, m[1], m[2], m[3], m[4]);
  else console.log(n, 'MISS');
}
// gui_portrait, gui_time_bg, gui_slot_bg, minimap
for (const n of ['gui_portrait_bg','gui_time_bg','gui_slot_bg','minimap_tile','minimap_player','gui_bleeding','gui_starving','gui_thirsty','gui_sick','gui_heat']) {
  const re = new RegExp('\\["images\\/' + n + '-sheet0\\.png",\\s*[^,]+,\\s*(\\d+),\\s*(\\d+),\\s*(\\d+),\\s*(\\d+)');
  const m = raw.match(re);
  if (m) console.log(n, m[3]+'x'+m[4], 'at', m[1]+','+m[2]);
}
