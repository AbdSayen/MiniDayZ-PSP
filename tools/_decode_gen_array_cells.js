const fs = require("fs");
const j = require("../cpp/docs_gen/functions/generate_array_locations.json");
const s = JSON.stringify(j);

// Find sequences: location type string near [511,182 SetAt value]
for (const typ of ["village", "city", "military", "gas", "hospital", "fire", "police", "church", "heli", "secret"]) {
  let from = 0;
  const vals = new Set();
  for (let n = 0; n < 8; n++) {
    const i = s.indexOf('"' + typ, from);
    if (i < 0) break;
    from = i + typ.length;
    const slice = s.slice(i, i + 2000);
    for (const m of slice.matchAll(/\[511,182,null[^\]]+\[7,\[0,(\d+)\]\]/g)) vals.add(+m[1]);
    for (const m of slice.matchAll(/\[10,0\],\[8,0\],\[7,\[0,(\d+)\]\]/g)) {
      const v = +m[1];
      if (v >= 5 && v <= 20) vals.add(v);
    }
  }
  if (vals.size) console.log(typ, [...vals].sort((a, b) => a - b).join(","));
}

// Count 507,71 Set IV0 values (array cell markers)
const cellVals = {};
let p = 0;
while ((p = s.indexOf("[507,71,null", p)) >= 0) {
  const slice = s.slice(p, p + 120);
  const m = slice.match(/\[10,0\],\[8,0\],\[7,\[0,(\d+)\]\]/);
  if (m) cellVals[m[1]] = (cellVals[m[1]] || 0) + 1;
  p++;
}
console.log("507 IV0 cell markers:", Object.entries(cellVals).sort((a, b) => a[0] - b[0]).map(([k, v]) => k + ":" + v).join(" "));
