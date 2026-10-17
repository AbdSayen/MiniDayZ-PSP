/**
 * Proven path: player_weapons fire → bullet 910 SetInstanceVar IV0 literal before spawn/New_bullet.
 * Maps C2 weapon tag strings → item IDs (151–187) and records unique IV0 literals per tag.
 */
const fs = require("fs");
const path = require("path");

const t = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "");

// C2 IV4 / fire function weapon tags → item id (1.4.1)
const tagToId = {
  fnx: 151,
  colt: 152,
  magnum: 153,
  izh: 154,
  remington: 155,
  mosin: 156,
  sks: 157,
  m4: 158,
  akm: 159,
  ak74: 160,
  aks74u: 161,
  l85: 162,
  bow: 163,
  crossbow: 164,
  amphibia: 165,
  sporter: 166,
  repeater: 167,
  svd: 168,
  mp5: 169,
  glock: 170,
  pm: 181,
  pb: 182,
  bizon: 183,
  groza: 184,
  vss: 185,
  sv98: 186,
  madsen: 187,
  saiga: 178,
  mac10: 179,
  ump45: 173,
  aug: 177,
  rpk: 174,
  fncal: 175,
};

const proven = {};
for (const [tag, itemId] of Object.entries(tagToId)) {
  const values = new Set();
  let from = 0;
  let n = 0;
  while (n < 40) {
    const i = t.indexOf('"' + tag + '"', from);
    if (i < 0) break;
    from = i + tag.length + 1;
    const slice = t.slice(Math.max(0, i - 300), Math.min(t.length, i + 2000));
    // Must be in shooting context: 910 bullet or New_bullet or player_weapons 909
    if (!slice.includes("910") && !slice.includes("909") && !slice.includes("New_bullet")) {
      n++;
      continue;
    }
    for (const mm of slice.matchAll(/910,71,null[^\]]+\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 5 && v <= 120) values.add(v);
    }
    for (const mm of slice.matchAll(/910,88,null[^\]]+\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 5 && v <= 120) values.add(v);
    }
    // Alternate: SetInstanceVar IV0 on bullet in same window as tag
    for (const mm of slice.matchAll(/910[^\]]{0,200}\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 5 && v <= 120) values.add(v);
    }
    n++;
  }
  if (values.size === 1) {
    proven[itemId] = { tag, iv0: [...values][0], path: "player_weapons fire window → 910 IV0 literal" };
  } else if (values.size > 1) {
    proven[itemId] = { tag, iv0: [...values].sort((a, b) => a - b), ambiguous: true };
  }
}

console.log("=== Single IV0 (sverka candidate) ===");
for (const [id, p] of Object.entries(proven).sort((a, b) => a[0] - b[0])) {
  if (!p.ambiguous) console.log("id", id, p.tag, "IV0=", p.iv0);
}
console.log("\n=== Ambiguous / none ===");
for (const [tag, id] of Object.entries(tagToId).sort((a, b) => a[1] - b[1])) {
  if (!proven[id] || proven[id].ambiguous) {
    console.log("id", id, tag, proven[id] ? "candidates " + proven[id].iv0.join(",") : "(no 910 IV0 in fire window)");
  }
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "weapon_iv0_sverka.json");
fs.writeFileSync(outPath, JSON.stringify({ tagToId, proven }, null, 2));
console.log("\nwrote", outPath);
