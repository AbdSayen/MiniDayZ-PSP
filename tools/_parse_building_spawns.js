const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

const layouts = {
  city: 2676642,
  city2: 2674327,
  city3: 2670400,
  barrack: 2685573,
  church: 2646648,
  firestation: 2657659,
  firestation2: 2658316,
};

function parseLayout(name, startAt) {
  const chunk = raw.slice(startAt, startAt + 120000);
  // layout tag object type before [2,"name"]
  const tagM = chunk.match(/\[(\d+),73,null,[^\]]+\[\[4,340\],\[5,\[2,"Buildings"\]\],\[7,\[0,3\]\]\]\],\[340,158,null,[^\]]+\[\[1,\[2,"/);
  const anchorType = tagM ? +tagM[1] : null;

  // all Buildings spawns: [BASE,73,...,[[4,CHILD],[5,[2,"Buildings"]],[7,[0,SLOT]]]]
  const spawns = [];
  for (const m of chunk.matchAll(
    /\[(\d+),73,null,[^\]]+\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g
  )) {
    const base = +m[1];
    const child = +m[2];
    const slot = +m[3];
    if (child === 340) continue; // layout marker
    spawns.push({ base, child, slot, childName: idxToPrefix[child] || '?' });
  }

  // Buildings_base spawns
  let baseType = null;
  const bm = chunk.match(
    /\[\d+,73,null,[^\]]+\[\[4,(\d+)\],\[5,\[2,"Buildings_base"\]\],\[7,\[0,0\]\]\]/
  );
  if (bm) baseType = +bm[1];

  // Position literals on Buildings_base via action 72: [BASE,72,null,...,[[0,[0,N]]]]
  const positions = [];
  if (baseType) {
    const posRe = new RegExp(
      '\\[' + baseType + ',72,null,[^\\]]+\\[\\[0,\\[0,(-?\\d+)\\]\\]\\]',
      'g'
    );
    for (const pm of chunk.matchAll(posRe)) positions.push(+pm[1]);
  }

  // w1/w2 width vars — [337,103,...]] relative offsets
  const widthPairs = [];
  for (const m of chunk.matchAll(
    /\[337,103,null,[^\]]+\[\[0,\[5,\[20,(\d+),59,false,null,\[\[2,"w2"\]\]\],\[20,\1,59,false,null,\[\[2,"w1"\]\]\]\]\],\[0,\[5,\[20,\1,60,false,null,\[\[2,"w2"\]\]\],\[20,\1,60,false,null,\[\[2,"w1"\]\]\]\]\]/g
  )) {
    widthPairs.push(+m[1]);
  }

  return {
    anchorType,
    anchorName: anchorType ? idxToPrefix[anchorType] : null,
    baseType,
    baseName: baseType ? idxToPrefix[baseType] : null,
    spawns,
    positions,
    widthPairs,
  };
}

for (const [name, at] of Object.entries(layouts)) {
  const p = parseLayout(name, at);
  console.log('\n###', name);
  console.log(' anchor', p.anchorType, p.anchorName);
  console.log(' base', p.baseType, p.baseName);
  console.log(' positions', p.positions);
  console.log(' spawns', p.spawns.length);
  const byChild = {};
  for (const s of p.spawns) {
    if (!byChild[s.childName]) byChild[s.childName] = 0;
    byChild[s.childName]++;
  }
  console.log(' children', byChild);
}

// Search police hospital layouts
for (const n of ['police', 'hospital', 'policestation', 'hostel', 'village', 'village1', 'village2', 'village3', 'village4', 'gasstation', 'gas_station']) {
  const i = raw.indexOf(`[1,[2,"${n}"]`);
  const i2 = raw.indexOf(`[2,"${n}"]`);
  if (i2 >= 0) console.log('found string', n, i2);
}
