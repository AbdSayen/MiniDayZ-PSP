const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const m of r.matchAll(/\[1,\[2,"([^"]+)"\]\],\[3,1\]\]/g)) {
  const n = m[1];
  if (/police|hospital|gas|hostel|school|super|vil|brown|green|yellow|red|military|barrack|shtab|church|fire|city|garage|piano|yard/i.test(n))
    console.log(n);
}
