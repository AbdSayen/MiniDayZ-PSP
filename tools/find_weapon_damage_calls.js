/**
 * Find CallFunction "NPC_get_damage" / melee weapon damage literals near weapon names.
 */
const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

function contextWindows(needle, max) {
  const out = [];
  let from = 0;
  while (out.length < max) {
    const i = text.indexOf(needle, from);
    if (i < 0) break;
    out.push(text.slice(Math.max(0, i - 100), Math.min(text.length, i + 350)).replace(/\s+/g, " "));
    from = i + needle.length;
  }
  return out;
}

const weapons = [
  "fireaxe", "pipewrench", "crowbar", "pitchfork", "pickaxe", "sledgehammer",
  "shovel", "bat", "Fist", "knife", "akm", "ak74", "mosin", "m4", "sks",
  "glock", "fnx", "svd", "ump", "mp5", "shotgun", "bow", "crossbow"
];

console.log("=== NPC_get_damage call sites (sample) ===");
contextWindows('"NPC_get_damage"', 8).forEach((w, i) => {
  const nums = [...w.matchAll(/\[0,(-?\d+(?:\.\d+)?)\]/g)].map((m) => m[1]);
  console.log(i, "nums", nums.slice(0, 25).join(","), "|", w.slice(0, 200));
});

console.log("\n=== Weapon name near damage-ish numbers ===");
for (const w of weapons) {
  const wins = contextWindows('"' + w + '"', 4);
  if (!wins.length) {
    // try without strict
    continue;
  }
  const allNums = [];
  wins.forEach((s) => {
    [...s.matchAll(/\[0,(-?\d+(?:\.\d+)?)\]/g)].forEach((m) => {
      const n = parseFloat(m[1]);
      if (n >= 5 && n <= 120) allNums.push(n);
    });
  });
  const uniq = [...new Set(allNums)].sort((a, b) => a - b);
  if (uniq.length) console.log(w, uniq.join(","));
}

// New_bullet function
console.log("\n=== New_bullet windows ===");
contextWindows('"New_bullet"', 5).forEach((w, i) => {
  const nums = [...w.matchAll(/\[0,(-?\d+(?:\.\d+)?)\]/g)].map((m) => m[1]);
  console.log(i, nums.slice(0, 30).join(","));
});
