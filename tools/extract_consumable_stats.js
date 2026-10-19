/**
 * Heuristic: near item name strings in Game_events, find SetInstanceVar on
 * player food/water/blood IVs (IV13/14/15) with literal adds.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

// Load item names
const items = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "cpp", "data", "items.json"), "utf8")).items;
const foodish = items.filter((it) => {
  const n = it.name.toLowerCase();
  return /bean|meat|bread|apple|soda|cola|water|pipsi|juice|can|rice|pasta|milk|whiskey|bandage|saline|blood|rags|morphine|vitamin|heatpack|orange|pepper|zucchini|cranberry|nota/i.test(n);
}).slice(0, 60);

const results = {};
for (const it of foodish) {
  const name = it.name;
  const hits = [];
  let from = 0;
  while (hits.length < 5) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) {
      // try short token
      break;
    }
    const slice = text.slice(Math.max(0, i - 50), Math.min(text.length, i + 800));
    // IV13 food, IV14 water, IV15 blood — SetInstanceVar / AddInstanceVar patterns
    const addIv = [...slice.matchAll(/\[10,(13|14|15)\].{0,60}\[0,(-?\d+(?:\.\d+)?)\]/g)];
    const nums = [...slice.matchAll(/\[0,(-?\d+(?:\.\d+)?)\]/g)].map((m) => parseFloat(m[1]))
      .filter((n) => n >= 1 && n <= 100);
    if (addIv.length || nums.length) {
      hits.push({
        ivAdds: addIv.map((m) => ({ iv: parseInt(m[1], 10), v: parseFloat(m[2]) })),
        nums: nums.slice(0, 12)
      });
    }
    from = i + name.length;
  }
  if (hits.length) results[it.id + ":" + name] = hits;
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "consumable_hints.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
console.log("wrote", out, "entries", Object.keys(results).length);
for (const [k, v] of Object.entries(results).slice(0, 25)) {
  const ivs = [];
  v.forEach((h) => h.ivAdds.forEach((a) => ivs.push("IV" + a.iv + "=" + a.v)));
  console.log(k, ivs.join(" ") || ("nums " + v[0].nums.slice(0, 6).join(",")));
}
