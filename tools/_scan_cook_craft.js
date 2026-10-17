const fs = require('fs');
const t = fs.readFileSync(require('path').join(__dirname, '../original_docs/data.js'), 'utf8');
const names = [
  'Craft_rags_bandage', 'Craft_shirt_rags', 'Craft_sticks_arrow', 'Craft_pile_sticks',
  'Craft_sticks_ashwood', 'Craft_spinning_rod', 'Craft_molotov', 'Craft_composite_arrow',
  'Craft_improvised_splint', 'Craft_torch_tinder', 'Craft_rags_rope', 'Craft_papers_matches',
  'cook', 'Cook', 'meat_cook', 'empty_click', 'Gun_jam', 'jammed', 'Wetness', 'wetness'
];
for (const n of names) {
  const i = t.indexOf(n);
  console.log(n, i >= 0 ? 'FOUND@' + i : 'MISSING');
}
// item_activate branches that spawn craft results?
const re = /"(Spawn_drop)"/g;
let c = 0, m;
while ((m = re.exec(t)) && c < 3) c++;
console.log('Spawn_drop refs rough count scan done');
