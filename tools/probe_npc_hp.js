/**
 * Probe layout IV patterns near zed/animal sprite names for HP candidates.
 */
const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const names = [
  "zed", "zombie", "wolf", "deer", "rabbit", "crow", "dog", "bandit", "bot",
  "zed_normal", "zed_fast", "zed_army", "zed_tank", "zed_screamer", "zed_buried"
];

for (const n of names) {
  const hits = [];
  let from = 0;
  while (hits.length < 4) {
    const i = text.toLowerCase().indexOf(n.toLowerCase(), from);
    if (i < 0) break;
    const slice = text.slice(i, Math.min(text.length, i + 500));
    // Instance var patterns: [0,N] near health-ish ranges 40-500
    const nums = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)]
      .map((m) => +m[1])
      .filter((v) => v >= 40 && v <= 500);
    if (nums.length) hits.push([...new Set(nums)].slice(0, 8));
    from = i + n.length;
  }
  if (hits.length) console.log(n, JSON.stringify(hits));
}
