/**
 * Try to recover items master Array (runtime obj536) column 301 (container capacity)
 * from C2 project layout initial state or bulk SetX events.
 */
const fs = require("fs");
const path = require("path");

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

const type536 = project[3][536];
const type533 = project[3][533];

// Clothing / backpack ids to report if found
const clothingIds = [
  300, 301, 302, 303, 304, 305, 306, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361,
  362, 363, 364, 365, 400, 401, 402, 403, 404, 405, 450, 451, 452, 453, 454, 455, 456, 457,
];

function walkDeep(v, fn, d = 0) {
  if (d > 25) return;
  fn(v, d);
  if (Array.isArray(v)) v.forEach((x) => walkDeep(x, fn, d + 1));
}

// Scan entire project JSON for numeric grids: rows keyed by item id at col0, col301 value
const s = JSON.stringify(project);
const col301ByRow = {};
// Pattern: row index then later 301 - too fragile. Instead find sequences [itemId, ..., cap] in array init blobs.

// Look for Array plugin instances in all layouts (type index 536)
const array536Instances = [];
for (const lay of project[5]) {
  walkDeep(lay, (v) => {
    if (!Array.isArray(v) || v.length < 8) return;
    // C2 instance: [x,y,w,h,angle, typeIndex, ...]
    if (v[5] === 536) array536Instances.push({ layout: lay[0], inst: v });
  });
}

// Scan event sheets for SetX on picked 536 with literal column 301
const set301 = [];
const reSet =
  /\[536,157,null[^\]]+\[0,\[0,(\d+)\]\]\],[^\[]+\[7,\[0,(\d+(?:\.\d+)?)\]\]/g;
let m;
while ((m = reSet.exec(s))) {
  if (+m[1] === 301) set301.push({ row: null, col: 301, val: +m[2] });
}

// Broader: Array SetAt 511,182 with row from [0,ITEMID] and col 301
const capByItem = {};
const reAt =
  /\[511,182,null[^\]]+\[10,0\],\[8,0\],\[7,\[0,(\d+)\]\][^\]]+\[10,1\],\[8,0\],\[7,\[0,301\]\][^\]]+\[7,\[0,(\d+(?:\.\d+)?)\]\]/g;
while ((m = reAt.exec(s))) {
  capByItem[m[1]] = +m[2];
}

const out = {
  type536_plugin: type536 ? type536[1] : null,
  type533_plugin: type533 ? type533[1] : null,
  array536_layout_instances: array536Instances.length,
  set301_event_hits: set301.length,
  cap_by_item_from_setat: capByItem,
  clothing_ids_missing: clothingIds.filter((id) => capByItem[String(id)] === undefined),
};

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "clothing_capacity_col301.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
console.log("cap entries", Object.keys(capByItem).length);
console.log("sample", Object.entries(capByItem).slice(0, 15));
