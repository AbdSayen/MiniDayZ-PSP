const fs = require("fs");
const t = fs.readFileSync(require("path").join(__dirname, "..", "original_docs", "data.js"), "utf8");

let n = 0;
let idx = 0;
while (n < 25) {
  const i = t.indexOf("910,88,", idx);
  if (i < 0) break;
  const slice = t.slice(i, i + 200);
  if (slice.indexOf("[10,0]") >= 0) {
    console.log(n, slice.replace(/\s+/g, " ").slice(0, 180));
    n++;
  }
  idx = i + 5;
}
console.log("done, scanned sets with 910,88");

// Melee damage via NPC_get_damage param from player melee
n = 0;
idx = 0;
console.log("\nMelee CallFunction NPC_get_damage near fireaxe/Fist:");
while (n < 10) {
  const i = t.indexOf('"Fist"', idx);
  if (i < 0) break;
  console.log(t.slice(i - 50, i + 250).replace(/\s+/g, " "));
  console.log("---");
  idx = i + 5;
  n++;
}
