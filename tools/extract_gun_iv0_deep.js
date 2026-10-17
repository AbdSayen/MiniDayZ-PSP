/**
 * Deep scan: SetInstanceVar IV0 near gun sprite / type name strings in Game_events.
 * Candidate base damages for firearms (heuristic; mark as researched, not sverka).
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const guns = [
  ["fnx", "fnx_pistol", "FNX"],
  ["colt", "colt_pistol", "Colt"],
  ["magnum", "magnum_pistol", "Magnum"],
  ["glock", "glock_pistol", "Glock"],
  ["akm", "akm_rifle", "AKM"],
  ["ak74", "ak74_rifle", "AK-74"],
  ["aks", "aks74u", "AKs"],
  ["m4", "m4_rifle", "M4"],
  ["mosin", "mosin_nagant", "Mosin"],
  ["sks", "sks_rifle", "SKS"],
  ["svd", "svd_rifle", "SVD"],
  ["mp5", "mp5_smg", "MP5"],
  ["izh", "bm16", "IZH"],
  ["remington", "r670", "Remington"],
  ["bow", "improvised_bow", "bow"],
  ["crossbow", "crossbow", "Crossbow"],
  ["amphibia", "amphibia", "Amphibia"],
  ["sporter", "sporter", "Sporter"],
  ["repeater", "repeater", "Repeater"],
  ["l85", "l85", "L85"],
  ["aug", "aug_rifle", "AUG"],
  ["saiga", "saiga", "Saiga"],
  ["mac10", "mac10", "MAC"],
  ["ump", "ump45", "UMP"],
  ["fireaxe", "fireaxe", "fireaxe"],
  ["sword", "sword", "sword"],
  ["fist", "Fist", "Fist"],
];

const out = {};
for (const aliases of guns) {
  const key = aliases[0];
  const iv0 = new Set();
  const nums = new Set();
  for (const name of aliases) {
    let from = 0;
    let n = 0;
    while (n < 10) {
      const i = text.indexOf('"' + name + '"', from);
      if (i < 0) {
        const j = text.indexOf(name, from);
        if (j < 0) break;
        from = j + name.length;
        const slice = text.slice(Math.max(0, j - 80), Math.min(text.length, j + 900));
        for (const mm of slice.matchAll(/\[10,0\].{0,80}\[0,(\d+(?:\.\d+)?)\]/g))
          iv0.add(parseFloat(mm[1]));
        for (const mm of slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)) {
          const v = parseFloat(mm[1]);
          if (v >= 8 && v <= 150) nums.add(v);
        }
        n++;
        continue;
      }
      from = i + name.length;
      const slice = text.slice(Math.max(0, i - 80), Math.min(text.length, i + 900));
      for (const mm of slice.matchAll(/\[10,0\].{0,80}\[0,(\d+(?:\.\d+)?)\]/g))
        iv0.add(parseFloat(mm[1]));
      for (const mm of slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)) {
        const v = parseFloat(mm[1]);
        if (v >= 8 && v <= 150) nums.add(v);
      }
      n++;
    }
  }
  out[key] = {
    setIv0: [...iv0].sort((a, b) => a - b),
    nums: [...nums].sort((a, b) => a - b).slice(0, 40),
  };
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "gun_iv0_deep.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
for (const [k, v] of Object.entries(out)) {
  if (v.setIv0.length || v.nums.length)
    console.log(k, "IV0=", v.setIv0.join("|") || "-", "nums=", v.nums.slice(0, 12).join(","));
}
