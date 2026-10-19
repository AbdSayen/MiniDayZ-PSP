/**
 * Heuristic scan of Game_events for numeric literals near weapon-related strings.
 * Not a full decompiler — produces candidates for manual verification.
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const weapons = [
  "FNX", "Glock", "Magnum", "AKM", "AK-74", "M4", "Mosin", "SKS", "SVD",
  "MP5", "UMP", "IZH", "Remington", "bow", "Crossbow", "Wolf_dmg", "Zed_dmg"
];

const results = {};
for (const w of weapons) {
  const idx = [];
  let from = 0;
  while (true) {
    const i = text.indexOf(w, from);
    if (i < 0) break;
    idx.push(i);
    from = i + w.length;
    if (idx.length > 40) break;
  }
  const windows = idx.slice(0, 15).map((i) => {
    const slice = text.slice(Math.max(0, i - 80), Math.min(text.length, i + 120));
    const nums = [...slice.matchAll(/:(-?\d+(?:\.\d+)?)/g)].map((m) => m[1]);
    return { nums: nums.slice(0, 20), preview: slice.replace(/\s+/g, " ").slice(0, 160) };
  });
  results[w] = { hits: idx.length, windows };
}

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, "weapon_hints.json"), JSON.stringify(results, null, 2));
console.log("Wrote weapon_hints.json");
for (const w of weapons) {
  console.log(w, "hits=", results[w].hits);
}
