/**
 * Find Game_events blocks that invoke Craft_* and list [0,[0,itemId]] compares.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const items = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "docs_gen", "items_eng.json"), "utf8")
).items;
const itemName = Object.fromEntries(items.map((i) => [i.id, i.name]));

const craftFns = [
  "Craft_1",
  "Craft_3",
  "Craft_4",
  "Craft_5",
  "Craft_21",
  "Craft_22",
  "Craft_31",
  "Craft_32",
  "Craft_33",
  "Craft_34",
  "Craft_fishing_rod",
];

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    for (const fn of craftFns) {
      if (s.includes('"Craft_1"') && fn === "Craft_1" && !s.includes('"Craft_1"')) {
      }
      if (!s.includes('"' + fn + '"')) continue;
      const itemIds = [...s.matchAll(/\[0,\[0,(\d+)\]\]/g)].map((m) => parseInt(m[1], 10));
      const objHdr = [...s.matchAll(/\[20,536,119,false,null,\[\[0,(\d+)\]\]\]/g)].map((m) =>
        parseInt(m[1], 10)
      );
      if (itemIds.length || objHdr.length) {
        out.push({
          fn,
          len: s.length,
          itemIds: [...new Set(itemIds)].sort((a, b) => a - b),
          itemNames: [...new Set(itemIds)]
            .sort((a, b) => a - b)
            .map((id) => id + ":" + (itemName[id] || "?")),
          headerObjIdx: [...new Set(objHdr)].sort((a, b) => a - b),
        });
      }
    }
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
walk(game[1], hits);

// Dedupe by fn + item set
const seen = new Set();
const uniq = [];
for (const h of hits.sort((a, b) => a.len - b.len)) {
  const key = h.fn + "|" + h.itemIds.join(",") + "|" + h.headerObjIdx.join(",");
  if (seen.has(key)) continue;
  seen.add(key);
  uniq.push(h);
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "craft_call_sites.json");
fs.writeFileSync(outPath, JSON.stringify(uniq, null, 2));
console.log("wrote", outPath, "entries", uniq.length);
for (const u of uniq) {
  console.log(u.fn, "items:", u.itemNames.join(", "), "hdr:", u.headerObjIdx.join(","));
}
