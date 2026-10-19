const fs = require("fs");
const path = require("path");
const game = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project[6].find((s) => s[0] === "Game_events");
const t = JSON.stringify(game);

const proven = {};
for (let itemId = 151; itemId <= 187; itemId++) {
  const needle = "[20,172," + itemId + ",false,null]";
  const values = new Set();
  let from = 0;
  while (true) {
    const i = t.indexOf(needle, from);
    if (i < 0) break;
    from = i + needle.length;
    const slice = t.slice(Math.max(0, i - 200), Math.min(t.length, i + 450));
    // IV0 set on bullet in same sub-event (88 = SetInstanceVar actions)
    if (!slice.includes(",88,null")) continue;
    for (const mm of slice.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
  }
  if (values.size) proven[itemId] = [...values].sort((a, b) => a - b);
}

console.log("Loose window (172 item + bullet 88 IV0 within 650 chars):");
for (const [id, vals] of Object.entries(proven).sort((a, b) => a[0] - b[0])) {
  console.log(id, vals.join("|"), vals.length === 1 ? "SVERKA" : "ambig");
}

fs.writeFileSync(
  path.join(__dirname, "..", "cpp", "docs_gen", "weapon_iv0_loose.json"),
  JSON.stringify(proven, null, 2)
);
