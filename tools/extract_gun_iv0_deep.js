/**
 * Deep scan: for each firearm display name / short token, find SetInstanceVar IV0
 * numeric assigns within 400 chars after the name hit in data.js.
 */
const fs = require("fs");
const path = require("path");

const data = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const guns = [
  ["fnx", 151], ["FNX", 151], ["1911", 152], ["Magnum", 153], ["IZH", 154],
  ["Remington", 155], ["Mosin", 156], ["SKS", 157], ["M4", 158], ["AKM", 159],
  ["AK-74", 160], ["AKs", 161], ["L85", 162], ["bow", 163], ["Crossbow", 164],
  ["Amphibia", 165], ["Sporter", 166], ["Repeater", 167], ["SVD", 168],
  ["MP5", 169], ["Glock", 170], ["UMP", 171], ["AUG", 177]
];

const out = {};
for (const [token, id] of guns) {
  const hits = [];
  let from = 0;
  while (hits.length < 15) {
    const i = data.indexOf('"' + token + '"', from);
    if (i < 0) break;
    const slice = data.slice(i, Math.min(data.length, i + 500));
    const sets = [...slice.matchAll(/\[10,0\][\s\S]{0,60}\[0,(\d+(?:\.\d+)?)\]/g)];
    for (const m of sets) {
      const v = parseFloat(m[1]);
      if (v >= 8 && v <= 120) hits.push(v);
    }
    from = i + token.length;
  }
  const uniq = [...new Set(hits)].sort((a, b) => a - b);
  if (uniq.length) out[id + ":" + token] = uniq;
}

const file = path.join(__dirname, "..", "cpp", "docs_gen", "gun_iv0_deep.json");
fs.writeFileSync(file, JSON.stringify(out, null, 2));
console.log("wrote", file);
for (const [k, v] of Object.entries(out)) console.log(k, v.join(","));
