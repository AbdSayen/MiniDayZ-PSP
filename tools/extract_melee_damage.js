/**
 * Search for melee weapon string IV4 compares near SubInstanceVar / NPC_get_damage damage literals.
 */
const fs = require("fs");
const path = require("path");
const t = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const melee = [
  "Fist", "fireaxe", "pipewrench", "crowbar", "pitchfork", "pickaxe",
  "sledgehammer", "shovel", "bat", "splitaxe", "sword", "knife", "knife_army",
  "knife_hunter", "knife_butcher"
];

const results = {};
for (const name of melee) {
  const hits = [];
  let from = 0;
  while (hits.length < 8) {
    const i = t.indexOf('"' + name + '"', from);
    if (i < 0) break;
    const slice = t.slice(Math.max(0, i - 120), Math.min(t.length, i + 400));
    // Look for NPC_get_damage params with literal damage
    const call = slice.match(/NPC_get_damage[^\]]*\[13,[^\]]*\[0,(\d+)/);
    const lit = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)].map((m) => parseFloat(m[1]));
    const dmgish = lit.filter((n) => n >= 8 && n <= 80);
    hits.push({ dmgish, hasCall: !!call, callDmg: call ? call[1] : null, preview: slice.slice(0, 160) });
    from = i + name.length;
  }
  results[name] = hits;
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "melee_damage_hints.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
for (const [k, v] of Object.entries(results)) {
  const nums = new Set();
  v.forEach((h) => h.dmgish.forEach((n) => nums.add(n)));
  if (nums.size) console.log(k, [...nums].sort((a, b) => a - b).join(","));
}
console.log("wrote", out);
