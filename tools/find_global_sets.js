const fs = require("fs");
const path = require("path");
const name = process.argv[2] || "Reloading_time";
const s = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");
const needle = '"' + name + '"';
let i = 0;
let n = 0;
while ((i = s.indexOf(needle, i + 1)) >= 0 && n < 25) {
  const ctx = s.slice(i - 100, i + 150);
  if (ctx.includes('"Reloading_time"') || ctx.includes(name)) {
    console.log("---", n, "pos", i);
    console.log(ctx.replace(/\s+/g, " "));
    n++;
  }
}
