const fs = require('fs');
let s = fs.readFileSync('original_docs/data.js', 'utf8');
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);

function contexts(needle, max = 20, before = 200, after = 500) {
  const out = [];
  let p = 0;
  while ((p = s.indexOf(needle, p)) >= 0 && out.length < max) {
    out.push(s.slice(p - before, p + needle.length + after).replace(/\s+/g, ' '));
    p += 1;
  }
  return out;
}

for (const h of contexts('Q_task_id', 25)) {
  if (h.includes('[11,"Q_task_id"]') && h.includes('[7,[0,'))
    console.log(h.slice(0, 900));
  console.log('---');
}

for (const h of contexts('Q_target_sid', 15)) {
  if (h.includes('[11,"Q_target_sid"]'))
    console.log('target:', h.slice(0, 700));
}
