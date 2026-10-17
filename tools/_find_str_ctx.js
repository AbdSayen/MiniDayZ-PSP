const fs = require('fs');
const needle = process.argv[2] || 'AltMove';
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

function walk(n, path, out) {
  if (typeof n === 'string' && n === needle) out.push(path);
  else if (Array.isArray(n)) n.forEach((x, i) => walk(x, path + '[' + i + ']', out));
  else if (n && typeof n === 'object') Object.keys(n).forEach((k) => walk(n[k], path + '.' + k, out));
}

const out = [];
walk(project, 'project', out);
console.log('hits', out.length);
out.slice(0, 15).forEach((p) => console.log(p));
