const fs = require("fs");
const path = require("path");
let s = fs.readFileSync(path.join(__dirname, "../original_docs/data.js"), "utf8");
if (s.charCodeAt(0) === 0xfeff) s = s.slice(1);
for (const fn of ["Reload_locations", "Reload_locations_start", "Reload_locations_aftersave"]) {
  let p = 0;
  let n = 0;
  console.log("\n===", fn, "===");
  while ((p = s.indexOf('"' + fn + '"', p)) >= 0 && n < 12) {
    const sn = s.slice(Math.max(0, p - 150), p + 120).replace(/\s+/g, " ");
    if (sn.includes("69,null") || sn.includes("40,null")) console.log(n, sn);
    n++;
    p++;
  }
  console.log("total", n);
}
