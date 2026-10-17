const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

function allTags(name) {
  const tag = `[1,[2,"${name}"]],[3,1]`;
  let pos = 0;
  const out = [];
  while (true) {
    const tagAt = raw.indexOf(tag, pos);
    if (tagAt < 0) break;
    const before = raw.slice(Math.max(0, tagAt - 800), tagAt);
    const anchorM = before.match(/\[(\d+),73,null,[^\]]+\[\[4,340\],\[5,\[2,"Buildings"\]\],\[7,\[0,3\]\]\]/);
    const anchor = anchorM ? idxToPrefix[+anchorM[1]] : null;
    const after = raw.slice(tagAt, tagAt + 8000);
    const caseM = after.match(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/);
    let spawns = [];
    if (caseM) {
      const caseStart = tagAt + after.indexOf(caseM[0]) + caseM[0].length;
      let block = raw.slice(caseStart, caseStart + 6000);
      const end = block.indexOf(']]]],[0,null,false,null,');
      if (end > 0) block = block.slice(0, end);
      for (const m of block.matchAll(/\[\[4,(\d+)\],\[5,\[2,"Buildings"\]\],\[7,\[0,(\d+)\]\]\]/g)) {
        const nm = idxToPrefix[+m[1]];
        if (nm && !nm.includes('_int') && +m[1] !== 340) spawns.push(nm + '@' + m[2]);
      }
    }
    out.push({ anchor, caseId: caseM ? +caseM[1] : null, spawns });
    pos = tagAt + tag.length;
  }
  return out;
}

for (const n of ['brown3', 'firestation', 'gas', 'brown1', 'brown2', 'white', 'city', 'city2', 'city3']) {
  const inst = allTags(n);
  if (inst.length) {
    console.log('\n' + n, inst.length, 'instances');
    for (const x of inst) console.log(' ', x);
  }
}

// gas station layout - search anchor 773
const i = raw.indexOf('[773,73,null,');
console.log('\ngas773 ctx', raw.slice(i, i + 500));
