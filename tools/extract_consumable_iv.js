/**
 * Find SetInstanceVar IV adds near food/drink item name strings (heuristic restores).
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const items = [
  [1, "Canned beans", "beans"],
  [2, "Canned tuna", "tuna"],
  [3, "Tactical bacon", "bacon"],
  [4, "Rice", "rice"],
  [5, "Tomato", "tomato"],
  [6, "Apple", "apple"],
  [7, "Banana", "banana"],
  [8, "Pipsi", "pipsi"],
  [9, "Spite", "spite"],
  [10, "Kvas", "kvas"],
  [18, "Bandage", "bandage"],
  [19, "Blood bag", "blood_bag"],
  [28, "Saline", "saline"],
  [45, "Morphine", "morphine"],
  [54, "Waterbottle", "waterbottle"],
  [55, "Canteen", "canteen"],
];

const out = {};
for (const [id, name, key] of items) {
  const hits = [];
  for (const term of [key, name]) {
    let from = 0;
    let n = 0;
    while (n < 6) {
      const i = text.toLowerCase().indexOf(String(term).toLowerCase(), from);
      if (i < 0) break;
      const slice = text.slice(i, Math.min(text.length, i + 1200));
      const ivAdds = [...slice.matchAll(/\[11,(\d+)\].{0,40}\[0,(\d+(?:\.\d+)?)\]/g)].map(
        (m) => ({ iv: +m[1], v: +m[2] })
      );
      const nums = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)]
        .map((m) => +m[1])
        .filter((v) => v >= 1 && v <= 100);
      if (ivAdds.length || nums.length) hits.push({ term, ivAdds: ivAdds.slice(0, 8), nums: nums.slice(0, 20) });
      from = i + term.length;
      n++;
    }
  }
  out[id + ":" + name] = hits;
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "consumable_iv_scan.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
for (const [k, hits] of Object.entries(out)) {
  const ivs = [];
  for (const h of hits) for (const a of h.ivAdds) ivs.push("IV" + a.iv + "=" + a.v);
  if (ivs.length) console.log(k, [...new Set(ivs)].slice(0, 10).join(", "));
}
