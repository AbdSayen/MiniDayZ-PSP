const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

const idx = parseInt(process.argv[2] || '946', 10);
const ot = project[3][idx];
console.log('type idx', idx, 'name', ot[0], 'plugin', ot[1]);
for (let i = 0; i < ot.length; i++) {
  console.log('[' + i + ']', typeof ot[i], Array.isArray(ot[i]) ? 'arr len ' + ot[i].length : String(ot[i]).slice(0, 80));
}
if (Array.isArray(ot[12])) {
  console.log('\nBehaviors / components [12]:');
  ot[12].forEach((b, bi) => console.log(' ', bi, JSON.stringify(b).slice(0, 120)));
}
// Family: search IV names in full JSON
const s = JSON.stringify(ot);
const ivNames = [...s.matchAll(/\[1,"([a-z_][a-z0-9_]*)",/gi)].map((m) => m[1]);
console.log('\nPossible IV names from [1,"name" pattern:', [...new Set(ivNames)].join(', '));
