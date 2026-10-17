const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const s = new Set();
for (const m of r.matchAll(/\[1,\[2,"([^"]{1,30})"\]\],\[3,1\]\]/g)) s.add(m[1]);
const all = [...s].sort();
console.log('total tags', all.length);
console.log(all.filter((x) => /vil|city|barrack|church|gas|fire|police|hosp|hostel|school|super|military|heli|secret|hunter|brown|green|red|yellow|yard|house|shtab|tent|garage|shed|piano/i.test(x)).join('\n'));
