const fs = require("fs");
const game = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project[6].find((s) => s[0] === "Game_events");
const t = JSON.stringify(game);

// After SetVar Reloading_time to literal, find preceding weapon compare
const times = new Map();
let from = 0;
while (true) {
  const i = t.indexOf('"Reloading_time"', from);
  if (i < 0) break;
  from = i + 10;
  const slice = t.slice(Math.max(0, i - 1500), i + 400);
  const setM = slice.match(/\[11,"Reloading_time"\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/);
  if (!setM) continue;
  const sec = parseFloat(setM[1]);
  const itemM = [...slice.matchAll(/172,(\d{3}),false,null/g)].map((m) => +m[1]);
  const tagM = [...slice.matchAll(/\[2,"(fnx|glock|akm|mosin|m4|colt|mp5)"\]/g)].map((m) => m[1]);
  const key = itemM.length ? "id:" + itemM[itemM.length - 1] : tagM.length ? tagM[tagM.length - 1] : "generic";
  if (!times.has(key)) times.set(key, new Set());
  times.get(key).add(sec);
}

console.log("Reloading_time by context:");
for (const [k, v] of [...times.entries()].sort()) {
  console.log(k, [...v].sort((a, b) => a - b).join(", "));
}

// Mag_reload: weapon IV0 reload ticks on 909
console.log("\n909 IV0=1 (reload sec?) count near Mag_reload:");
let c = 0;
from = 0;
while (c < 5) {
  const i = t.indexOf('"Mag_reload"', from);
  if (i < 0) break;
  from = i + 5;
  const slice = t.slice(i, i + 800);
  const iv = slice.match(/909,88[^\]]+\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/);
  if (iv) console.log("Mag_reload IV0", iv[1]);
  c++;
}
