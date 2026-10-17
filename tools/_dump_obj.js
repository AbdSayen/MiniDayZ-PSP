const fs = require('fs');
const ot = process.argv[2] || 't172';
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;
const obj = project[3].find((o) => o[0] === ot);
console.log(JSON.stringify(obj, null, 2).slice(0, 12000));
