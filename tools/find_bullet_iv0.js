/**
 * Find SetInstanceVar on bullet family (obj 910) IV0 = damage assignments near weapon strings.
 */
const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

// Pattern: 910,88,...[[10,0],[7,[0,DAMAGE]]]  = SetInstanceVar IV0 = DAMAGE
const re = /910,88,null,\d+,false,\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]\]/g;
const damages = {};
let m;
while ((m = re.exec(text))) {
  const d = m[1];
  damages[d] = (damages[d] || 0) + 1;
}
console.log("Bullet IV0 SetInstanceVar literal damages:");
Object.entries(damages)
  .sort((a, b) => parseFloat(a[0]) - parseFloat(b[0]))
  .forEach(([d, c]) => console.log(" ", d, "x" + c));

// Also find weapon string then nearby IV0 set within 500 chars
const weapons = [
  "fireaxe", "pipewrench", "crowbar", "pitchfork", "pickaxe", "sledgehammer",
  "shovel", "bat", "splitaxe", "akm", "ak74", "m4", "mosin", "sks", "svd",
  "glock", "fnx", "ump", "mp5", "l85", "shotgun", "izh", "bow", "crossbow",
  "magnum", "colt", "sporter", "repeater", "amphibia", "saiga", "aug", "rpk"
];

console.log("\nWeapon string → nearby [0,N] damage candidates (5-120):");
for (const w of weapons) {
  let from = 0;
  const found = new Set();
  for (let n = 0; n < 6; n++) {
    const i = text.indexOf('"' + w + '"', from);
    if (i < 0) break;
    const slice = text.slice(i, i + 800);
    const nums = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)].map((x) => parseFloat(x[1]));
    nums.filter((v) => v >= 5 && v <= 120).forEach((v) => found.add(v));
    // also IV0 sets
    const iv0 = [...slice.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)].map((x) => parseFloat(x[1]));
    iv0.forEach((v) => found.add(v));
    from = i + w.length;
  }
  if (found.size) console.log(w, [...found].sort((a, b) => a - b).join(","));
}
