// Helper: grep contexts in original_docs/data.js
const fs = require('fs');
const path = require('path');
let s = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);

function contexts(needle, max = 8, before = 280, after = 900) {
  const out = [];
  let p = 0;
  while ((p = s.indexOf(needle, p)) >= 0 && out.length < max) {
    out.push(s.slice(p - before, p + needle.length + after).replace(/\s+/g, ' '));
    p += 1;
  }
  return out;
}

const needles = process.argv.slice(2);
const list = needles.length
  ? needles
  : [
      'tilemap_water',
      'SetTile',
      'Bridges_2',
      'create_map_location',
      'Q_target_sid',
      'Q_task_id',
      'Generate_quest_bandits',
    ];
for (const needle of list) {
  const hits = contexts(needle, 6);
  console.log('\n=== ' + needle + ' (' + hits.length + ' shown) ===');
  hits.forEach((h, i) => console.log(i + ': ' + h.slice(0, 1400)));
}
