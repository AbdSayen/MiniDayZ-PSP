const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
// Find object type names that look like inventory items
const names = new Set();
const re = /"([a-z0-9_]*(?:fnx|akm|mosin|bandage|hatchet|knife|glock|sks|m4|axe|shovel|ammo_|wpn_|weapon_|item_)[a-z0-9_]*)"/gi;
let m;
while ((m = re.exec(raw))) names.add(m[1].toLowerCase());
console.log([...names].sort().slice(0, 80).join('\n'));

// Also list image files referenced that contain common weapon names
const imgs = new Set();
const ire = /images\/([a-z0-9_]+-(?:sheet\d+\.png|[a-z0-9_]+\.png))/gi;
let n = 0;
while ((m = ire.exec(raw)) && n < 50000) {
  const s = m[1].toLowerCase();
  if (/fnx|akm|bandage|hatchet|glock|mosin|sks|m4_|knife|shovel|crowbar|torch|canteen|beans|rice|vest|backpack|magnum|saiga|svd|mp5|aug/.test(s))
    imgs.add(s);
  n++;
}
console.log('---images---');
console.log([...imgs].sort().join('\n'));
