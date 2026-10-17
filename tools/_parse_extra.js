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
  const anchor = anchorM ? idxToPrefix[+anchorM[1]] : null;

  const after = raw.slice(tagAt, tagAt + 8000);
  const caseM = after.match(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/);
  if (!caseM) return { layoutName, anchor, caseId: null, spawns: [] };
  const caseStart = tagAt + after.indexOf(caseM[0]) + caseM[0].length;
  let block = raw.slice(caseStart, caseStart + 8000);
  const end = block.indexOf(']]]],[0,null,false,null,');
  if (end > 0) block = block.slice(0, end);
  const spawns = [];
  for (const m of block.matchAll(/\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g)) {
    const name = idxToPrefix[+m[1]];
    if (name && !name.includes('_int') && +m[1] !== 340) spawns.push(name + '@' + m[2]);
  }
  return { layoutName, anchor, caseId: +caseM[1], spawns };
}

for (const n of ['white', 'brown1', 'brown2', 'brown3', 'red2', 'ga_brown', 'shtab', 'barrack']) {
  console.log(extractSpawnBlock(n));
}

// hospital / police dedicated: search anchor 326 and 328 first tag after
for (const idx of [326, 328, 329, 773, 327, 320]) {
  const needle = `[${idx},73,null,`;
  const i = raw.indexOf(needle);
  if (i < 0) continue;
  const ctx = raw.slice(i, i + 400);
  const tagM = ctx.match(/\[1,\[2,"([^"]+)"\]\],\[3,1\]\]/);
  console.log('idx', idx, idxToPrefix[idx], 'tag', tagM && tagM[1]);
}
