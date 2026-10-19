/**
 * Heuristic: find mag-size-ish literals near gun name strings in Game_events.
 * Common mag sizes: 5,6,7,8,10,15,20,25,30,40,64
 */
const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const guns = [
  "fnx", "colt", "glock", "magnum", "akm", "ak-74", "aks", "m4", "mosin", "sks",
  "svd", "mp5", "pm", "vss", "bizon", "saiga", "aug", "ump"
];
const magish = new Set([5, 6, 7, 8, 10, 12, 15, 17, 20, 25, 30, 40, 45, 50, 64]);

for (const g of guns) {
  const counts = {};
  let from = 0;
  let hits = 0;
  while (hits < 12) {
    const i = text.toLowerCase().indexOf(g, from);
    if (i < 0) break;
    const slice = text.slice(i, Math.min(text.length, i + 800));
    for (const m of slice.matchAll(/\[0,(\d+)\]/g)) {
      const n = +m[1];
      if (magish.has(n)) counts[n] = (counts[n] || 0) + 1;
    }
    from = i + g.length;
    hits++;
  }
  const top = Object.entries(counts).sort((a, b) => b[1] - a[1]).slice(0, 6);
  if (top.length) console.log(g, top.map(([n, c]) => n + "×" + c).join(", "));
}
