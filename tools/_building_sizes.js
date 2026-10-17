const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;
const types = project[3];
function size(name) {
  const t = types.find((x) => x[0] === name);
  if (!t || !t[7] || !t[7][0] || !t[7][0][7] || !t[7][0][7][0]) return null;
  const f = t[7][0][7][0];
  return { w: f[4], h: f[5] };
}
const names = [
  'b_village_brown', 'b_village_green', 'b_village_red', 'b_village_yellow', 'b_village_yard', 'b_village_red2',
  'b_city_house_1', 'b_city_house_2', 'b_city_house_3', 'b_city_house_4', 'b_hostel', 'b_car_bus',
  'b_military_barrack', 'b_military_barrack2', 'b_military_shtab', 'b_hunter_shelter',
  'b_gas_station', 'b_garage_blue', 'b_garage_brown', 'b_hospital', 'b_fire_station', 'b_police_station',
  'b_church', 'b_supermarket', 'b_red_brick', 'b_piano_house',
];
for (const n of names) console.log(n, size(n));
