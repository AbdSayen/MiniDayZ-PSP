const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
function firstFrame(imgBase) {
  const re = new RegExp(imgBase + '-sheet0\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)');
  const m = raw.match(re);
  return m ? { x:+m[1], y:+m[2], w:+m[3], h:+m[4] } : null;
}
for (const n of [
  'fnx_pistol','akm_rifle','bandage','canned_beans','zed_bullet','spawn_loot_point',
  'knife_hunter','shovel','ammo_9mm','canteen','fireaxe','colt_pistol','glock_pistol',
  'm4_rifle','mosin_nagant','sks_rifle','improvised_bow','crossbow','crowbar','pickaxe',
  'baseball_bat','barbedwire','batteries','flare','morphine','rags','blood_bag'
]) {
  console.log(n, JSON.stringify(firstFrame(n)));
}
