const fs = require("fs");
const path = require("path");
const s = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");
const needle = "Bunker_spawn_zeds";
let i = 0;
let n = 0;
while ((i = s.indexOf(needle, i + 1)) >= 0 && n < 30) {
  console.log("---", n, s.slice(i - 60, i + 200).replace(/\s+/g, " "));
  n++;
}
