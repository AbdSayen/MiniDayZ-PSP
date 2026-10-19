const fs = require('fs');
const ot = process.argv[2] || 't946';
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;
const obj = project[3].find((o) => o[0] === ot);
if (!obj) {
  console.log('not found', ot);
  process.exit(1);
}
console.log('name', obj[0], 'plugin', obj[1]);
const beh = obj[7];
console.log('behaviors', JSON.stringify(beh, null, 2).slice(0, 4000));
