const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

function extractSpawnBlock(layoutName) {
  const tag = `[1,[2,"${layoutName}"]],[3,1]`;
  const tagAt = raw.indexOf(tag);
  if (tagAt < 0) return null;
  const before = raw.slice(Math.max(0, tagAt - 800), tagAt);
  const anchorM = before.match(/\[(\d+),73,null,[^\]]+\[\[4,340\],\[5,\[2,"Buildings"\]\],\[7,\[0,3\]\]\]/);
  const anchor = anchorM ? +anchorM[1] : null;

  const after = raw.slice(tagAt, tagAt + 20000);
  const caseM = after.match(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/);
  if (!caseM) return { layoutName, anchor, anchorName: anchor ? idxToPrefix[anchor] : null, caseId: null, spawns: [] };
  const caseStart = tagAt + after.indexOf(caseM[0]) + caseM[0].length;
  let block = raw.slice(caseStart, caseStart + 12000);
  const end = block.indexOf(']]]],[0,null,false,null,');
  if (end > 0) block = block.slice(0, end);

  const spawns = [];
  for (const m of block.matchAll(/\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g)) {
    const child = +m[1];
    const slot = +m[2];
    const name = idxToPrefix[child];
    if (!name || name.includes('_int') || child === 340) continue;
    spawns.push({ child, slot, name });
  }
  for (const m of block.matchAll(/\[\[4,(\d+)\],\[5,\[2,"Buildings_shadows"\]\],\[7,\[2,"([^"]+)"\]\]\]/g)) {
    spawns.push({ child: +m[1], slot: m[2], name: (idxToPrefix[+m[1]] || '?') + '_shadow' });
  }

  const offsets = [];
  for (const m of block.matchAll(/\[337,103,null,[^\]]+\[\[0,\[5,\[20,(\d+),59,false,null,\[\[2,"w2"\]\]\],\[20,\1,59,false,null,\[\[2,"w1"\]\]\]\]\],\[0,\[5,\[20,\1,60,false,null,\[\[2,"w2"\]\]\],\[20,\1,60,false,null,\[\[2,"w1"\]\]\]\]\]/g)) {
    offsets.push({ base: +m[1], baseName: idxToPrefix[+m[1]] });
  }

  return {
    layoutName,
    anchor,
    anchorName: anchor ? idxToPrefix[anchor] : null,
    caseId: +caseM[1],
    spawns,
    offsets,
  };
}

const layouts = [
  'brown1', 'brown2', 'brown3', 'Green', 'Yellow', 'yellow', 'red', 'red2', 'Redneck', 'ga_brown',
  'city', 'city2', 'city3',
  'barrack', 'shtab',
  'church', 'gas', 'firestation', 'firestation2',
];

for (const name of layouts) {
  const r = extractSpawnBlock(name);
  if (!r) {
    console.log(name, 'NOT FOUND');
    continue;
  }
  console.log(
    JSON.stringify({
      layout: name,
      caseId: r.caseId,
      anchor: r.anchorName,
      spawns: r.spawns,
    })
  );
}
