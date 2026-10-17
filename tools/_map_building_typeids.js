const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;
const types = project[3];
const offs = JSON.parse(
  fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/poi_building_offsets.json'), 'utf8')
);
const ids = new Set();
for (const arr of Object.values(offs.byTag || {})) {
  for (const e of arr) if (e.typeId != null) ids.add(e.typeId);
}
const rows = [...ids]
  .sort((a, b) => a - b)
  .map((id) => {
    const t = types[id];
    return { id, name: t ? t[0] : '?' };
  });
console.log(JSON.stringify(rows, null, 2));
fs.writeFileSync(
  path.join(__dirname, '../cpp/docs_gen/poi_building_typeids.json'),
  JSON.stringify({ count: rows.length, types: rows }, null, 2)
);
