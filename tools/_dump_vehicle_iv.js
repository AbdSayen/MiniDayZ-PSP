const fs = require('fs');
const project = JSON.parse(
  fs.readFileSync('original_docs/data.js', 'utf8').replace(/^\uFEFF/, '')
).project;

const names = ['car_uaz_base', 'car_volga_base', 'car_truck', 'car_resp_uaz', 'car_resp_volga', 'Vehicle'];
for (const ot of project[7]) {
  if (!Array.isArray(ot) || typeof ot[1] !== 'string') continue;
  if (!names.some((n) => ot[1] === n || ot[1].includes(n))) continue;
  console.log('===', ot[1], '===');
  // IVs usually in ot[19] or ot[18] in C2 exports — dump numeric indices with string labels
  for (let i = 0; i < ot.length; i++) {
    const v = ot[i];
    if (Array.isArray(v) && v.length > 0 && typeof v[0] === 'string' && /fuel|hp|speed|condition|trunk|type/i.test(JSON.stringify(v))) {
      console.log('slot', i, JSON.stringify(v).slice(0, 800));
    }
  }
  const full = JSON.stringify(ot);
  const m = full.match(/"fuel[^"]*"|"hp[^"]*"|"condition[^"]*"|"speed[^"]*"/gi);
  if (m) console.log('strings:', [...new Set(m)].join(', '));
  // instance variable table often index 19: [[name, desc, type], ...]
  if (Array.isArray(ot[19])) {
    console.log('IV table (ot[19]):');
    ot[19].forEach((iv, idx) => {
      if (Array.isArray(iv) && iv[0]) console.log(' ', idx, iv[0], iv.slice(1, 4));
    });
  }
  console.log('');
}
