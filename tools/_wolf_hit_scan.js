const fs = require("fs");
const t = fs.readFileSync("original_docs/data.js", "utf8");
const re = /Player_get_hit[\s\S]{0,120}\[7,\[2,"wolf"\]\],\[7,\[23,"(\w+)"\]\]/g;
const c = {};
let m;
while ((m = re.exec(t))) c[m[1]] = (c[m[1]] || 0) + 1;
console.log("wolf hit globals:", c);

// Wolf_dmg_deal attack frame context
const idx = t.indexOf("Wolf_dmg_deal");
console.log("Wolf_dmg_deal at", idx);
console.log(t.slice(idx, idx + 2500));
