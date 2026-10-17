/**
 * Sverka path: New_bullet spawn template sets IV1←player item id (172,151) and IV0←literal
 * on spawned bullet instance (obj 177, 250, 549, …) in same action list.
 */
const fs = require("fs");
const path = require("path");
const game = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project[6].find((s) => s[0] === "Game_events");
const t = JSON.stringify(game);

const proven = {};
const re =
  /\[(\d+),88,null,\d+,false,\[\[10,1\],\[7,\[20,172,(\d{3}),false,null\]\]\]\],\[(\d+),88,null,\d+,false,\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]\]/g;

let m;
while ((m = re.exec(t)) !== null) {
  const itemId = parseInt(m[2], 10);
  const dmg = parseFloat(m[4]);
  if (itemId < 151 || itemId > 210 || dmg < 5 || dmg > 120) continue;
  if (!proven[itemId]) proven[itemId] = new Set();
  proven[itemId].add(dmg);
}

console.log("=== bullet spawn IV0 ← item id (172,151) ===");
for (const id of Object.keys(proven).sort((a, b) => a - b)) {
  const vals = [...proven[id]].sort((a, b) => a - b);
  const status = vals.length === 1 ? "SVERKA" : "AMBIG";
  console.log(status, "item", id, "IV0", vals.join("|"));
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "weapon_bullet_iv0_sverka.json");
fs.writeFileSync(
  out,
  JSON.stringify(
    Object.fromEntries(
      Object.entries(proven).map(([id, set]) => [
        id,
        {
          iv0: [...set].sort((a, b) => a - b),
          sverka: set.size === 1,
          path: "Game_events: bullet 88 Set IV1←172.itemId + IV0 literal (New_bullet spawn chain)",
        },
      ])
    ),
    null,
    2
  )
);
console.log("wrote", out);
