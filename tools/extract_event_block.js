// Extract C2 event subtree by event uid from original_docs/data.js
const fs = require('fs');
const path = require('path');
let s = fs.readFileSync(path.join(__dirname, '../original_docs/data.js'), 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);

const uid = process.argv[2] || '6938246402369311';
const idx = s.indexOf(uid);
console.log('uid index', idx);
if (idx < 0) process.exit(1);
// Walk back to start of event array [0,null,false,null,UID,
let start = idx;
while (start > 0 && s.slice(start - 5, start) !== 'null,') start--;
start = s.lastIndexOf('[0,null,false,null,', idx);
console.log('block start', start);
// Bracket match from start
let depth = 0;
let end = start;
for (let i = start; i < s.length; i++) {
  if (s[i] === '[') depth++;
  else if (s[i] === ']') {
    depth--;
    if (depth === 0) {
      end = i + 1;
      break;
    }
  }
}
const block = s.slice(start, end);
console.log('block length', block.length);
const outPath = path.join(__dirname, '../cpp/docs_gen/river_event_block.json');
fs.writeFileSync(outPath, block);
console.log('wrote', outPath);

// Quick scan for keywords
for (const kw of ['river', 'River', 'Bridge', 'tilemap_water', 'SetTile', '511,', '199,', 'check_X', 'random', 'loop']) {
  const re = new RegExp(kw, 'gi');
  const n = (block.match(re) || []).length;
  if (n) console.log(kw, n);
}
