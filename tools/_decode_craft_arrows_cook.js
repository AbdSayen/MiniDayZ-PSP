const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;

function walk(evts, fn) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    fn(e, s);
    for (const x of e) walk(x, fn);
  }
}

let arrows = '';
let meat = '';
project[6].forEach((sh) =>
  walk(sh[1], (e, s) => {
    if (s.includes('Craft arrows') && s.length > arrows.length) arrows = s;
    if (s.includes('meat_cook') && s.length > meat.length && s.length < 200000) meat = s;
  })
);

function extractNums(s) {
  const nums = [...s.matchAll(/\[0,(\d+)\]/g)].map((m) => +m[1]);
  const counts = {};
  for (const n of nums) {
    if (n >= 1 && n <= 500) counts[n] = (counts[n] || 0) + 1;
  }
  return Object.entries(counts)
    .sort((a, b) => b[1] - a[1])
    .slice(0, 40)
    .map(([n, c]) => n + '×' + c);
}

console.log('Craft arrows body len', arrows.length);
console.log('Craft arrows top nums', extractNums(arrows).join(', '));
console.log('sample', arrows.slice(0, 400));
console.log('---');
console.log('meat_cook body len', meat.length);
console.log('meat_cook top nums', extractNums(meat).join(', '));
console.log('sample', meat.slice(0, 400));

fs.writeFileSync(path.join(__dirname, '../cpp/docs_gen/craft_arrows_cook_scan.txt'),
  '# Craft arrows / meat_cook scan\n\n' +
  'Craft arrows len ' + arrows.length + '\n' + extractNums(arrows).join(', ') + '\n\n' +
  'meat_cook len ' + meat.length + '\n' + extractNums(meat).join(', ') + '\n');
