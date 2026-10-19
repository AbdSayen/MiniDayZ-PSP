const fs = require("fs");
const t = fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "");

const tagToId = {
  fnx: 151, colt: 152, magnum: 153, izh: 154, remington: 155, mosin: 156, sks: 157,
  m4: 158, akm: 159, ak74: 160, aks74u: 161, l85: 162, bow: 163, crossbow: 164,
  amphibia: 165, sporter: 166, repeater: 167, svd: 168, mp5: 169, glock: 170,
  pm: 181, pb: 182, bizon: 183, groza: 184, vss: 185, sv98: 186, madsen: 187,
  saiga: 178, mac10: 179, ump45: 173, aug: 177, rpk: 174, fncal: 175,
};

function iv0Literals(slice) {
  const out = new Set();
  for (const mm of slice.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
    const v = parseFloat(mm[1]);
    if (v >= 5 && v <= 120) out.add(v);
  }
  return out;
}

const proven = {};
for (const [tag, itemId] of Object.entries(tagToId)) {
  const values = new Set();
  let from = 0;
  for (let n = 0; n < 50; n++) {
    const i = t.indexOf('"' + tag + '"', from);
    if (i < 0) break;
    from = i + tag.length + 1;
    const slice = t.slice(Math.max(0, i - 500), Math.min(t.length, i + 2500));
    if (!slice.includes("909") && !slice.includes("910") && !slice.includes("New_bullet")) continue;
    iv0Literals(slice).forEach((v) => values.add(v));
  }
  if (values.size === 1) proven[itemId] = { tag, iv0: [...values][0] };
  else if (values.size > 1) proven[itemId] = { tag, iv0: [...values].sort((a, b) => a - b), ambiguous: true };
}

console.log("Single IV0:");
for (const [id, p] of Object.entries(proven).filter(([, p]) => !p.ambiguous).sort((a, b) => a[0] - b[0])) {
  console.log(id, p.tag, p.iv0);
}
console.log("\nAmbiguous:");
for (const [id, p] of Object.entries(proven).filter(([, p]) => p.ambiguous).sort((a, b) => a[0] - b[0])) {
  console.log(id, p.tag, p.iv0.join("|"));
}
console.log("\nMissing tags:");
for (const [tag, id] of Object.entries(tagToId).sort((a, b) => a[1] - b[1])) {
  if (!proven[id]) console.log(id, tag);
}
