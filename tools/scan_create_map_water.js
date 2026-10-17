const fs = require('fs');
let s = fs.readFileSync('original_docs/data.js', 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);
const uid = '8627807469654645';
const start = s.lastIndexOf('[0,null,false,null,', s.indexOf(uid));
let depth = 0,
  end = start;
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
// Sub-block only for type 4
const t4 = block.indexOf('4962273709016567');
const sub = block.slice(t4, t4 + 25000);
// Find 945,185 with last param 4 (water) - pattern [0,[0,4]],[3,0]] at end before ]]
let p = 0,
  c = 0;
while ((p = sub.indexOf('[945,185,null', p)) >= 0) {
  const chunk = sub.slice(p, p + 400);
  // tile id is 5th numeric param before [3,0]
  const m = chunk.match(/\[945,185,null,\d+,false,\[\[[^\]]+\]\],\[0,\[0,(\d+)\]\],\[0,\[0,(\d+)\]\],\[0,\[0,(\d+)\]\],\[3,0\]\]/);
  if (m) {
    console.log('range', m[1], 'x', m[2], 'y', m[3], 'tile', m[4] || '?');
  } else if (chunk.includes('[0,[0,4]],[3,0]') || chunk.match(/\[0,\[0,\d+\]\],\[0,\[0,\d+\]\],\[0,\[0,4\]\]/)) {
    console.log('water?', chunk.slice(0, 280).replace(/\s+/g, ' '));
    c++;
  }
  p++;
}
console.log('945,185 total', (sub.match(/\[945,185,null/g) || []).length);

// 199 set tile
p = 0;
while ((p = sub.indexOf('[199,', p)) >= 0 && c < 20) {
  console.log('199 act:', sub.slice(p, p + 180).replace(/\s+/g, ' '));
  p++;
  c++;
}
