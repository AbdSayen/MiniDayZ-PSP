const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

function blockForTag(layoutName) {
  const tag = `[1,[2,"${layoutName}"]],[3,1]`;
  const tagAt = raw.indexOf(tag);
  if (tagAt < 0) return null;
  return raw.slice(tagAt - 400, tagAt + 25000);
}

for (const name of ['yellow', 'Yellow', 'Green', 'red', 'Redneck']) {
  const b = blockForTag(name);
  if (!b) {
    console.log(name, 'missing');
    continue;
  }
  const anchorM = b.match(/\[(\d+),73,null,[^\]]+\[\[4,340\],\[5,\[2,"Buildings"\]\],\[7,\[0,3\]\]\]/);
  console.log('\n', name, 'anchor', anchorM ? idxToPrefix[+anchorM[1]] : 'none');
  const cases = [...b.matchAll(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/g)].map((m) => m[1]);
  console.log(' cases', cases);
  const buildings = [...b.matchAll(/\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g)]
    .map((m) => idxToPrefix[+m[1]] + '@' + m[2])
    .filter((x) => x && !x.startsWith('undefined'));
  console.log(' buildings in window', buildings.slice(0, 15));
}

// Find all layout tags where anchor is police/hospital/gas
const anchorsWanted = {
  b_police_station: 328,
  b_hospital: 326,
  b_gas_station: 773,
  b_village_brown: 322,
  b_village_green: 321,
  b_village_yellow: 323,
  b_village_yard: 330,
};

for (const [prefix, idx] of Object.entries(anchorsWanted)) {
  const needle = `[${idx},73,null,`;
  let pos = 0;
  while (true) {
    const i = raw.indexOf(needle, pos);
    if (i < 0) break;
    const ctx = raw.slice(i, i + 600);
    const tagM = ctx.match(/\[1,\[2,"([^"]+)"\]\],\[3,1\]\]/);
    if (tagM) console.log('anchor', prefix, 'layout tag', tagM[1]);
    pos = i + 1;
  }
}
