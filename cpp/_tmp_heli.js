const fs = require('fs');
const r = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
for (const n of ['heli_crash','helicrash_point','crosshair_rds','crosshair_acog','crosshair_pso','crosshair_pu','crosshair_long']) {
  const m = r.match(new RegExp(n + '-sheet0\\.png\",\\d+,(\\d+),(\\d+),(\\d+),(\\d+)'));
  console.log(n, m ? m.slice(1).join(',') : 'MISS');
}
