const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

for (const idx of [773, 772, 774, 326, 294]) {
  let pos = 0;
  while (true) {
    const i = raw.indexOf(`[${idx},73,null,`, pos);
    if (i < 0) break;
    const ctx = raw.slice(i - 200, i + 400);
    const tagM = ctx.match(/\[1,\[2,"([^"]+)"\]\],\[3,1\]\]/);
    console.log(idx, idxToPrefix[idx], 'tag', tagM && tagM[1], 'at', i);
    pos = i + 1;
    if (pos > i + 1e6) break;
  }
}
