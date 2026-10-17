/**
 * Heuristic: find item id literals near "new game" / inventory seed strings in Game_events.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const anchors = [
  "SEED RUNS",
  "Start events",
  "gamesav",
  "fromsav",
  "Novice",
  "newgame",
  "Start_inv",
  "start_inv",
  "Give_item",
  "Add_item",
];

const out = {};
for (const a of anchors) {
  const i = text.indexOf(a);
  if (i < 0) {
    out[a] = null;
    continue;
  }
  const slice = text.slice(i, Math.min(text.length, i + 4000));
  const nums = [...slice.matchAll(/\[0,(\d+)\]/g)]
    .map((m) => parseInt(m[1], 10))
    .filter((n) => n >= 1 && n <= 500);
  // Compress runs of item-like IDs
  const freq = {};
  for (const n of nums) freq[n] = (freq[n] || 0) + 1;
  out[a] = {
    at: i,
    top: Object.entries(freq)
      .sort((x, y) => y[1] - x[1])
      .slice(0, 40)
      .map(([id, c]) => ({ id: +id, c })),
  };
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "starter_hints.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
for (const [k, v] of Object.entries(out)) {
  if (!v) {
    console.log(k, "MISS");
    continue;
  }
  console.log(
    k,
    v.top
      .slice(0, 15)
      .map((x) => x.id + "×" + x.c)
      .join(", ")
  );
}
