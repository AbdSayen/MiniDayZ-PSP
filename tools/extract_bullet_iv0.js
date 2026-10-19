/**
 * Heuristic: find SetInstanceVar IV0 numeric assigns near gun/melee name strings
 * in Game_events — candidates for bullet base damage.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const names = [
  "FNX", "fnx", "Glock", "Magnum", "AKM", "AK-74", "AKs", "M4", "Mosin", "SKS", "SVD",
  "MP5", "IZH", "Remington", "Crossbow", "bow", "Sporter", "Amphibia", "Repeater",
  "L85", "Colt", "fireaxe", "pipewrench", "crowbar", "sword", "Fist", "bat", "shovel"
];

// Pattern fragments in JSON: "IV0" near SetInstanceVar and a number
const results = {};
for (const name of names) {
  const hits = [];
  let from = 0;
  while (hits.length < 12) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) break;
    const slice = text.slice(Math.max(0, i - 100), Math.min(text.length, i + 500));
    // Look for SetInstanceVar with IV0 and literal
    const setIv0 = [...slice.matchAll(/\[10,0\].{0,40}\[0,(\d+(?:\.\d+)?)\]/g)];
    const anyNums = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)]
      .map((x) => parseFloat(x[1]))
      .filter((n) => n >= 8 && n <= 150);
    if (setIv0.length || anyNums.length) {
      hits.push({
        setIv0: setIv0.map((x) => parseFloat(x[1])),
        nums: anyNums.slice(0, 15)
      });
    }
    from = i + name.length;
  }
  results[name] = hits;
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "bullet_iv0_hints.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
console.log("wrote", out);
for (const name of names) {
  const all = new Set();
  for (const h of results[name]) {
    h.setIv0.forEach((n) => all.add(n));
    h.nums.forEach((n) => all.add(n));
  }
  if (all.size) console.log(name, [...all].sort((a, b) => a - b).join(","));
}
