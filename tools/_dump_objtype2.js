const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

const idx = parseInt(process.argv[2] || '946', 10);
const ot = project[3][idx];
console.log(JSON.stringify(ot, null, 2).slice(0, 12000));
