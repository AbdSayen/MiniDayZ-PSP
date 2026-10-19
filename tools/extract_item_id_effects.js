/**
 * Search for item ID literals near player IV13/14/15 Add/SetInstanceVar.
 * Looks for patterns like CompareInstanceVar IV0 == itemId then nearby blood/food adds.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const t = fs.readFileSync(dataPath, "utf8");

const itemIds = [1, 2, 4, 6, 8, 18, 25, 26, 27, 28, 30, 31, 33, 34, 45, 54];
const results = {};

for (const id of itemIds) {
  const hits = [];
  // Compare instance var with item id often: [0,ID] near inventory checks
  const needle = "[0," + id + "]";
  let from = 0;
  while (hits.length < 8) {
    const i = t.indexOf(needle, from);
    if (i < 0) break;
    const slice = t.slice(i, Math.min(t.length, i + 500));
    // Look for IV13/14/15 modifies
    const iv = [...slice.matchAll(/\[10,(13|14|15)\].{0,40}\[0,(-?\d+(?:\.\d+)?)\]/g)];
    if (iv.length) {
      hits.push(iv.map((m) => ({ iv: +m[1], v: +m[2] })));
    }
    from = i + needle.length;
  }
  if (hits.length) results[id] = hits;
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "item_id_effects.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
console.log("wrote", out);
for (const [id, hits] of Object.entries(results)) {
  const flat = [];
  hits.forEach((h) => h.forEach((a) => flat.push("IV" + a.iv + "=" + a.v)));
  console.log("id", id, flat.slice(0, 10).join(" "));
}
