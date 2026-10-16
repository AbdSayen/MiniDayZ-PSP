const fs = require("fs");
const t = fs.readFileSync(require("path").join(__dirname, "..", "original_docs", "data.js"), "utf8");
let from = 0;
let n = 0;
while (n < 15) {
  const i = t.indexOf('"Night_cold"', from);
  if (i < 0) break;
  console.log(t.slice(i - 40, i + 100).replace(/\s+/g, " "));
  console.log("---");
  from = i + 1;
  n++;
}
