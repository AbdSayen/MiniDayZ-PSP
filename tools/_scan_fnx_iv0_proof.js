const fs = require("fs");
const t = fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "");

function windows(needle, max, span) {
  const out = [];
  let from = 0;
  while (out.length < max) {
    const i = t.indexOf(needle, from);
    if (i < 0) break;
    out.push(t.slice(Math.max(0, i - span.before), Math.min(t.length, i + span.after)));
    from = i + needle.length;
  }
  return out;
}

console.log("=== fnx near SetInstanceVar IV0 literal ===");
for (const w of windows('"fnx"', 15, { before: 400, after: 600 })) {
  const iv0 = [...w.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)].map((m) => m[1]);
  if (iv0.length) console.log("IV0=", iv0.join(","), "|", w.replace(/\s+/g, " ").slice(0, 280));
}

console.log("\n=== CompareInstanceVar IV1 item 151 near IV0 on 910 ===");
for (const w of windows("172,151,false,null", 8, { before: 800, after: 400 })) {
  const iv0 = [...w.matchAll(/910[^\]]{0,100}\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)].map((m) => m[1]);
  const expr = w.includes("E[21,910") || w.includes("910,false,null,0");
  if (iv0.length || expr) {
    console.log("IV0=", iv0.join(",") || "(expr)", "|", w.replace(/\s+/g, " ").slice(-350));
  }
}

console.log("\n=== New_bullet function refs ===");
let nb = 0;
let from = 0;
while (nb < 5) {
  const i = t.indexOf("New_bullet", from);
  if (i < 0) break;
  from = i + 10;
  const sn = t.slice(i - 80, i + 120).replace(/\s+/g, " ");
  if (sn.includes("CallFunction") || sn.includes("[2,")) {
    console.log(nb, sn);
    nb++;
  }
}

console.log("\n=== Bullets_parameters group: spawn bullet IV0 ===");
const i = t.indexOf("Bullets parameters");
if (i >= 0) {
  const chunk = t.slice(i, i + 50000);
  const iv0all = [...chunk.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)].map((m) => m[1]);
  const uniq = [...new Set(iv0all)].sort((a, b) => a - b);
  console.log("IV0 literals in first 50k after group name:", uniq.slice(0, 40).join(", "), "count", uniq.length);
}

// player_weapons 909 IV0 reload?
console.log("\n=== 909 IV0 reload ticks (weapon reload IV) ===");
const reloadIv = new Map();
from = 0;
let n = 0;
while (n < 30) {
  const j = t.indexOf("909,88,null", from);
  if (j < 0) break;
  from = j + 5;
  const slice = t.slice(j, j + 120);
  if (!slice.includes("[10,0]")) continue;
  const iv = slice.match(/\[10,(\d+)\]/);
  const val = slice.match(/\[7,\[0,(\d+(?:\.\d+)?)\]\]/);
  if (iv && val && iv[1] === "0") {
    const v = parseFloat(val[1]);
    if (v >= 0.5 && v <= 5) reloadIv.set(v, (reloadIv.get(v) || 0) + 1);
  }
  n++;
}
console.log([...reloadIv.entries()].sort((a, b) => a[0] - b[0]).map(([k, c]) => k + " x" + c).join(", "));
