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
  // find case trigger after tag
  const after = raw.slice(tagAt, tagAt + 15000);
  const caseM = after.match(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/);
  if (!caseM) return { tagAt, caseId: null, spawns: [] };
  const caseId = +caseM[1];
  const caseStart = tagAt + after.indexOf(caseM[0]) + caseM[0].length;
  // block starts with ],[[ actions
  let block = raw.slice(caseStart, caseStart + 8000);
  const end = block.indexOf(']]]],[0,null,false,null,');
  if (end > 0) block = block.slice(0, end);

  const spawns = [];
  for (const m of block.matchAll(
    /\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g
  )) {
    const child = +m[1];
    const slot = +m[2];
    if (idxToPrefix[child] && !idxToPrefix[child].includes('_int')) {
      spawns.push({ child, slot, name: idxToPrefix[child] });
    }
  }
  // also capture anchor type from before tag
  const before = raw.slice(Math.max(0, tagAt - 500), tagAt);
  const anchorM = before.match(/\[(\d+),73,null,[^\]]+\[\[4,340\],\[5,\[2,"Buildings"\]\],\[7,\[0,3\]\]\]/);
  const anchor = anchorM ? +anchorM[1] : null;

  // X positions on spawned helpers: TYPE,72 with [0,[0,N]]
  const positions = [];
  for (const m of block.matchAll(/\[(\d+),72,null,[^\]]+\[\[0,\[0,(-?\d+)\]\]\]/g)) {
    positions.push({ obj: +m[1], name: idxToPrefix[+m[1]] || '?', v: +m[2] });
  }

  return { tagAt, caseId, anchor, anchorName: anchor ? idxToPrefix[anchor] : null, spawns, positions };
}

const names = [];
for (const m of raw.matchAll(/\[1,\[2,"([a-z0-9_]+)"\]\],\[3,1\]\]/g)) {
  if (m[1].length <= 20) names.push(m[1]);
}
const uniq = [...new Set(names)].filter((n) =>
  /city|vill|barrack|church|gas|fire|police|hospital|hostel|school|military|heli|secret|hunter|super/i.test(n)
);
console.log('layout tags found:', uniq.sort().join(', '));

for (const name of uniq.sort()) {
  const r = extractSpawnBlock(name);
  if (!r || !r.spawns.length) continue;
  console.log('\n', name, 'case', r.caseId, 'anchor', r.anchorName);
  console.log('  buildings', r.spawns.map((s) => s.name + '@slot' + s.slot).join(', '));
  if (r.positions.length) console.log('  pos', r.positions.slice(0, 8));
}
