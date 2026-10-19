const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

const needle = process.argv[2] || 'car_uaz_base';
for (let pi = 0; pi < project.length; pi++) {
  const part = project[pi];
  if (typeof part === 'string' && part.includes(needle)) {
    console.log('project[' + pi + '] string contains', needle);
  }
  if (!Array.isArray(part)) continue;
  for (let i = 0; i < part.length; i++) {
    const s = JSON.stringify(part[i]);
    if (s && s.includes('"' + needle + '"')) {
      console.log('project[' + pi + '][' + i + ']', s.slice(0, 200));
    }
  }
}
