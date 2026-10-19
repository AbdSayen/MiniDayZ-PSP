const fs = require("fs");
const path = require("path");
const game = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

// All literal IV0 sets on bullet type 910
const re = /910,88,null,\d+,false,\[\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]\]/g;
const contexts = [];
let m;
while ((m = re.exec(text)) !== null) {
  const i = m.index;
  const before = text.slice(Math.max(0, i - 2500), i);
  const after = text.slice(i, Math.min(text.length, i + 800));
  const dmg = parseFloat(m[1]);

  const itemIds = [
    ...before.matchAll(/172,151,false,null[^\]]{0,80}\[0,(\d{3})\]/g),
    ...before.matchAll(/CompareInstanceVar[^\]]{0,40}151[^\]]{0,40}\[0,(\d{3})\]/g),
  ].map((x) => parseInt(x[1], 10));

  const iv4 = [...before.matchAll(/"fnx"|"colt"|"akm"|"m4"|"mosin"|"glock"|"sks"|"svd"|"mp5"|"magnum"/gi)].map(
    (x) => x[0].replace(/"/g, "")
  );
  const nearFn = [...before.matchAll(/\[2,"([^"]{3,40})"\]/g)]
    .map((x) => x[1])
    .filter((n) => /single|fire|shoot|fnx|glock|akm|mosin|New_bullet/i.test(n))
    .slice(-3);

  contexts.push({ dmg, itemIds: [...new Set(itemIds)], iv4: [...new Set(iv4)], nearFn });
}

const byDmg = {};
for (const c of contexts) {
  if (!byDmg[c.dmg]) byDmg[c.dmg] = [];
  byDmg[c.dmg].push(c);
}
console.log("910 IV0 literal sets:", contexts.length);
for (const d of Object.keys(byDmg).sort((a, b) => a - b)) {
  console.log(" dmg", d, "count", byDmg[d].length);
  const sample = byDmg[d][0];
  console.log("   sample items", sample.itemIds, "iv4", sample.iv4, "fn", sample.nearFn);
}

// Map item id 151-187: look for [0,ID] within 400 chars before 910 IV0 set
const byItem = {};
for (const c of contexts) {
  for (const id of c.itemIds) {
    if (!byItem[id]) byItem[id] = new Set();
    byItem[id].add(c.dmg);
  }
  if (c.iv4.includes("fnx")) {
    if (!byItem[151]) byItem[151] = new Set();
    byItem[151].add(c.dmg);
  }
}
console.log("\nBy item id (from 172 IV1 or fnx tag):");
for (const id of Object.keys(byItem).sort((a, b) => a - b)) {
  console.log(" ", id, [...byItem[id]].sort((a, b) => a - b).join(", "));
}
