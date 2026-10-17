const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

for (const lay of project[5]) {
  const name = lay[0];
  const layers = lay[4];
  if (!layers) continue;
  for (const layer of layers) {
    const inst = layer[4];
    if (!inst) continue;
    for (const i of inst) {
      const typeId = i[1];
      if (typeId !== 184 && typeId !== 172) continue;
      const beh = i[9];
      if (!beh) continue;
      const s = JSON.stringify(beh);
      if (s.includes('AltMove') || s.length > 50) {
        console.log('layout', name, 'type', typeId, 'beh', s.slice(0, 800));
      }
    }
  }
}
