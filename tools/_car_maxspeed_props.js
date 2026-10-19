const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function findBeh(node, out, name) {
  if (!Array.isArray(node)) return;
  if (node.length >= 8 && typeof node[0] === 'string' && node[0].startsWith('t')) {
    const beh = node[13];
    if (Array.isArray(beh)) {
      for (const b of beh) {
        if (Array.isArray(b) && b[0] === 'Car') out.push({ type: node[0], props: b.slice(7) });
      }
    }
  }
  for (const x of node) findBeh(x, out, name);
}

const out = [];
for (const o of project[3]) {
  if (!Array.isArray(o)) continue;
  const beh = o[13];
  if (!Array.isArray(beh)) continue;
  for (const b of beh) {
    if (Array.isArray(b) && b[0] === 'Car') {
      out.push({ type: o[0], props: b.slice(7, 15) });
    }
  }
}
console.log('Car types', out.length);
out.forEach((x) => console.log(x.type, 'maxspeed,acc,dec,...', x.props));

// Also EightDir on player t172
const p172 = project[3].find((o) => o[0] === 't172');
if (p172 && Array.isArray(p172[13])) {
  for (const b of p172[13]) {
    if (Array.isArray(b) && b[0] === 'EightDirection') console.log('player EightDir', b.slice(7, 15));
    if (Array.isArray(b) && b[0] === 'EightDir') console.log('player EightDir', b.slice(7, 15));
  }
}
