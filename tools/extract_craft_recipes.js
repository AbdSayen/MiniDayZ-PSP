/**
 * Extract Craft_* group ingredient/result hints from data.js Game_events.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const craftNames = [
  "Craft_1", "Craft_21", "Craft_22", "Craft_3", "Craft_31", "Craft_32",
  "Craft_33", "Craft_34", "Craft_4", "Craft_5", "Craft_fishing_rod"
];

const results = {};
for (const name of craftNames) {
  const hits = [];
  let from = 0;
  while (hits.length < 6) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) break;
    const slice = text.slice(i, Math.min(text.length, i + 1200));
    const nums = [...slice.matchAll(/\[0,(\d+)\]/g)].map((m) => parseInt(m[1], 10))
      .filter((n) => n >= 1 && n <= 500);
    // Unique preserve order
    const uniq = [];
    for (const n of nums) {
      if (uniq.indexOf(n) < 0) uniq.push(n);
    }
    hits.push({ nums: uniq.slice(0, 40), preview: slice.slice(0, 200).replace(/\s+/g, " ") });
    from = i + name.length;
  }
  results[name] = hits;
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "craft_hints.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
console.log("wrote", out);
for (const name of craftNames) {
  const all = new Set();
  for (const h of results[name]) h.nums.forEach((n) => all.add(n));
  console.log(name, "hits=" + results[name].length, "ids=", [...all].slice(0, 25).join(","));
}
