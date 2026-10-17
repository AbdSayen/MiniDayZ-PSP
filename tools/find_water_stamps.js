const fs = require('fs');
let s = fs.readFileSync('original_docs/data.js', 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);
// In create_map_location: 511,186 compare at offset with value 4, then set water
const re = /\[511,186,null,[^\]]+\[\[0,[^\]]+\],\[0,[^\]]+\],\[8,0\],\[7,\[0,4\]\]\]\],[^\[]*\[\[199,/g;
let m, c = 0;
while ((m = re.exec(s))) {
  console.log('match', c++, m[0].slice(0, 200).replace(/\s+/g, ' '));
}
console.log('511==4 then 199', c);

// Any 199,185 or 199,187 SetTile with tile 4
let p = 0;
c = 0;
while ((p = s.indexOf('[199,185,null', p)) >= 0 && c < 15) {
  const ch = s.slice(p, p + 350);
  if (ch.includes('[0,[0,4]]') || ch.includes(',[0,[0,4]],[3,0]')) {
    console.log('199 set water:', ch.replace(/\s+/g, ' ').slice(0, 320));
    c++;
  }
  p++;
}
console.log('199,185 water sets', c);
