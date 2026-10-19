const fs = require('fs');
let s = fs.readFileSync('original_docs/data.js', 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);
// 511,182 set at check with value 4
const needle = '[511,182,null';
let p = 0,
  hits = 0;
while ((p = s.indexOf(needle, p)) >= 0) {
  const chunk = s.slice(p, p + 350);
  if (chunk.includes('[7,[0,4]]')) {
    console.log('type4 set:', chunk.replace(/\s+/g, ' ').slice(0, 320));
    hits++;
  }
  p++;
}
console.log('total type4 511,182', hits);

// any 511,182 with various values - collect unique [7,[0,N]]
const vals = {};
p = 0;
while ((p = s.indexOf(needle, p)) >= 0) {
  const chunk = s.slice(p, p + 280);
  const m = chunk.match(/\[7,\[0,(\d+)\]\]\]/);
  if (m) vals[m[1]] = (vals[m[1]] || 0) + 1;
  p++;
}
console.log('511,182 value histogram:', vals);
