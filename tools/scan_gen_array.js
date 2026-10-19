const fs = require('fs');
const b = fs.readFileSync('cpp/docs_gen/river_event_block.json', 'utf8');
let p = 0,
  c = 0;
while ((p = b.indexOf('[7,[0,4]]', p)) >= 0) {
  console.log('---', c, b.slice(p - 350, p + 150).replace(/\s+/g, ' '));
  p++;
  c++;
}
