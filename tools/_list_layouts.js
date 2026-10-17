const fs = require('fs');
const path = require('path');
const raw = fs.readFileSync(path.join(path.resolve(__dirname, '..'), 'original_docs/data.js'), 'utf8');
const project = JSON.parse(raw.slice(raw.indexOf('{'))).project;
for (const l of project[5]) {
  const n = l[0];
  if (/city|vill|brown|barrack|church|gas|fire|police|hosp|military|hostel|school|Map|location/i.test(n))
    console.log(n);
}
