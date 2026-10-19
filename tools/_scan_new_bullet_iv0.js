/**
 * Event-path scan: before each CallFunction "New_bullet", find bullet (910) SetInstanceVar IV0
 * and weapon id on player (172 IV1) or IV4 weapon string.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const game = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project[6].find(
  (s) => s[0] === "Game_events"
);
const text = JSON.stringify(game);

const hits = [];
let idx = 0;
while (hits.length < 500) {
  const i = text.indexOf("New_bullet", idx);
  if (i < 0) break;
  idx = i + 10;
  const slice = text.slice(Math.max(0, i - 2000), i + 100);
  if (!slice.includes("910")) continue;

  const iv0Sets = [
    ...slice.matchAll(/910,88,null,\d+,false,\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]\]/g),
  ].map((m) => parseFloat(m[1]));
  const iv0Sets2 = [...slice.matchAll(/910[^\]]{0,80}\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)].map(
    (m) => parseFloat(m[1])
  );
  const allIv0 = [...new Set([...iv0Sets, ...iv0Sets2])];

  const weaponIds = [
    ...slice.matchAll(/172,151,false,null[^\]]{0,60}\[0,(\d+)\]/g),
    ...slice.matchAll(/\[7,\[0,(\d{3})\]\]/g),
  ]
    .map((m) => parseInt(m[1], 10))
    .filter((id) => id >= 151 && id <= 210);

  const iv4 = [...slice.matchAll(/"IV4"[^\]]{0,80}"([^"]{2,30})"/g)].map((m) => m[1]);
  const fnx = slice.includes('"fnx"') || slice.includes('"FNX"');

  if (allIv0.length || weaponIds.length) {
    hits.push({
      iv0: allIv0,
      weaponIds: [...new Set(weaponIds)],
      iv4: [...new Set(iv4)],
      fnx,
    });
  }
}

const byWeapon = {};
for (const h of hits) {
  for (const d of h.iv0) {
    const key = h.weaponIds.length ? "id:" + h.weaponIds.join("+") : h.fnx ? "fnx" : h.iv4.join("|") || "?";
    if (!byWeapon[key]) byWeapon[key] = new Set();
    byWeapon[key].add(d);
  }
}

console.log("New_bullet+910 hits", hits.length);
for (const [k, v] of Object.entries(byWeapon).sort()) {
  console.log(k, [...v].sort((a, b) => a - b).join(", "));
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "new_bullet_iv0_paths.json");
fs.writeFileSync(outPath, JSON.stringify({ hits: hits.slice(0, 80), byWeapon: Object.fromEntries(
  Object.entries(byWeapon).map(([k, v]) => [k, [...v].sort((a, b) => a - b)])
) }, null, 2));
console.log("wrote", outPath);
