const fs = require("fs");
const game = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project[6].find((s) => s[0] === "Game_events");
const t = JSON.stringify(game);

let from = 0;
let n = 0;
while (n < 20) {
  const i = t.indexOf("[7,[0,23]]", from);
  if (i < 0) break;
  from = i + 10;
  const slice = t.slice(Math.max(0, i - 800), i + 400);
  if (!slice.includes("fnx") && !slice.includes("FNX") && !slice.includes("910")) continue;
  console.log("--- hit", n, "---");
  console.log(slice.replace(/\s+/g, " ").slice(0, 900));
  n++;
}

console.log("\n=== [0,23] after [10,0] near fnx (Game_events) ===");
from = 0;
n = 0;
while (n < 15) {
  const i = t.indexOf('"fnx"', from);
  if (i < 0) break;
  from = i + 5;
  const slice = t.slice(i, i + 1500);
  const m = slice.match(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/);
  if (m) {
    console.log("fnx → IV0", m[1], slice.slice(0, 400).replace(/\s+/g, " "));
    n++;
  }
}
