/**
 * Strong path: CompareInstanceVar IV4 == "weapontag" → SetInstanceVar IV0 on bullet 910.
 */
const fs = require("fs");
const t = fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "");

const tags = [
  "fnx", "colt", "magnum", "glock", "akm", "ak74", "aks74u", "m4", "mosin", "sks", "svd",
  "mp5", "izh", "remington", "bow", "crossbow", "amphibia", "sporter", "repeater", "l85",
  "aug", "saiga", "mac10", "ump45", "rpk", "fncal", "pm", "pb", "bizon", "groza", "vss", "sv98", "madsen",
];

const tagToId = {
  fnx: 151, colt: 152, magnum: 153, izh: 154, remington: 155, mosin: 156, sks: 157,
  m4: 158, akm: 159, ak74: 160, aks74u: 161, l85: 162, bow: 163, crossbow: 164,
  amphibia: 165, sporter: 166, repeater: 167, svd: 168, mp5: 169, glock: 170,
  pm: 181, pb: 182, bizon: 183, groza: 184, vss: 185, sv98: 186, madsen: 187,
  saiga: 178, mac10: 179, ump45: 173, aug: 177, rpk: 174, fncal: 175,
};

const proven = {};
for (const tag of tags) {
  const needle = '[7,[2,"' + tag + '"]]';
  const values = new Set();
  let from = 0;
  while (true) {
    const i = t.indexOf(needle, from);
    if (i < 0) break;
    from = i + needle.length;
    // IV4 compare blocks often followed by actions within ~3k
    const slice = t.slice(Math.max(0, i - 400), Math.min(t.length, i + 3500));
    if (!slice.includes("910")) continue;
    for (const mm of slice.matchAll(/910,71,null[^\]]+\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
    for (const mm of slice.matchAll(/910,88,null[^\]]+\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
  }
  const id = tagToId[tag];
  if (values.size === 1) proven[id] = { tag, iv0: [...values][0] };
  else if (values.size > 1) proven[id] = { tag, iv0: [...values].sort((a, b) => a - b), ambiguous: true };
}

console.log(JSON.stringify(proven, null, 2));
