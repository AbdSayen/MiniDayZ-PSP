const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const text = JSON.stringify(
  JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project[6].find(
    (s) => s[0] === "Game_events"
  )
);

const guns = [
  ["pm", 181],
  ["pb", 182],
  ["bizon", 183],
  ["bizon_smg", 183],
  ["groza", 184],
  ["groza_rifle", 184],
  ["vss", 185],
  ["vss_rifle", 185],
  ["sv98", 186],
  ["sv98_rifle", 186],
  ["sv-98", 186],
  ["madsen", 187],
  ["madsen_mg", 187],
  ["fnx", 151],
  ["fnx_pistol", 151],
];

for (const [g, id] of guns) {
  const values = new Set();
  let from = 0;
  let n = 0;
  while (n < 20) {
    const i = text.indexOf('"' + g + '"', from);
    if (i < 0) break;
    from = i + g.length + 1;
    const slice = text.slice(Math.max(0, i - 250), Math.min(text.length, i + 1500));
    for (const mm of slice.matchAll(/\[10,0\].{0,100}\[0,(\d+(?:\.\d+)?)\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
    for (const mm of slice.matchAll(/IV0.{0,80}\[0,(\d+(?:\.\d+)?)\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
    n++;
  }
  console.log(id, g, [...values].sort((a, b) => a - b).join(", ") || "(none)");
}

// Item id 181-187 as CompareInstanceVar weapon id
for (const id of [181, 182, 183, 184, 185, 186, 187]) {
  const values = new Set();
  let from = 0;
  while (true) {
    const needle = "[0,[0," + id + "]]";
    const i = text.indexOf(needle, from);
    if (i < 0) break;
    from = i + needle.length;
    const slice = text.slice(Math.max(0, i - 100), Math.min(text.length, i + 800));
    for (const mm of slice.matchAll(/\[10,0\].{0,100}\[0,(\d+(?:\.\d+)?)\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 8 && v <= 120) values.add(v);
    }
  }
  console.log("itemId", id, [...values].sort((a, b) => a - b).join(", ") || "(none)");
}
