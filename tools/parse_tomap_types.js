const fs = require('fs');
const block = fs.readFileSync('cpp/docs_gen/river_event_block.json', 'utf8');
const parts = block.split('[511,207,null');
for (let i = 1; i < parts.length; i++) {
  const chunk = parts[i].slice(0, 400);
  const idxM = chunk.match(/\[\[8,0\],\[7,\[0,(\d+)\]\]\]/);
  const idx = idxM ? idxM[1] : '?';
  const sub = parts[i].slice(0, 1200);
  const valM = sub.match(/\[507,88,null,[^\]]+\[\[10,0\],\[7,\[19,82,\[\[([^\]]+)\]\]\]\]\]/);
  const val = valM ? valM[1] : sub.match(/507,88/) ? sub.slice(sub.indexOf('507,88'), sub.indexOf('507,88') + 250) : 'no 507';
  console.log('array index', idx, '->', val);
}
