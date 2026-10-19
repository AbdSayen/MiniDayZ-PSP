/**
 * Parse item_activate_decoded.txt → literal IV13/14/15/16 adds per item id (param 0).
 */
const fs = require("fs");
const path = require("path");

const src = path.join(__dirname, "..", "cpp", "docs_gen", "functions", "item_activate_decoded.txt");
const lines = fs.readFileSync(src, "utf8").split(/\r?\n/);

const reParam = /CompareParam \[\[0,0\],\[8,0\],(\d+)\]/;
const reAdd = /AddInstanceVar \["IV(\d+)",(-?\d+(?:\.\d+)?)\]/;
const reSetIv = /SetInstanceVar \["IV(\d+)",(-?\d+(?:\.\d+)?)\]/;

const out = {};
let cur = null;
let depth = 0;

for (const line of lines) {
  const pm = line.match(reParam);
  if (pm) {
    cur = +pm[1];
    if (!out[cur]) out[cur] = { iv13: null, iv14: null, iv15: null, iv16: null, notes: [] };
    depth = 1;
    continue;
  }
  if (cur == null) continue;
  if (/^IF obj180 fn\.cnd\.CompareParam/.test(line) && !reParam.test(line)) {
    // nested param — still same item block until next top-level param
  }
  const am = line.match(reAdd);
  if (am && depth) {
    const iv = +am[1];
    const v = parseFloat(am[2]);
    const key = "iv" + iv;
    if (out[cur][key] == null) out[cur][key] = v;
  }
  const sm = line.match(reSetIv);
  if (sm && depth && (sm[1] === "13" || sm[1] === "14" || sm[1] === "15" || sm[1] === "16")) {
    const key = "iv" + sm[1];
    out[cur][key] = parseFloat(sm[2]);
    out[cur].notes.push("SetInstanceVar IV" + sm[1] + "=" + sm[2]);
  }
  if (/CompareParam \[\[0,0\],\[8,0\],\d+\]/.test(line) && !line.includes(String(cur))) {
    // next item — handled by pm at start
  }
}

const items = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "data", "items.json"), "utf8")
).items;
const byId = Object.fromEntries(items.map((it) => [it.id, it.name]));

const enriched = {};
for (const [id, v] of Object.entries(out)) {
  enriched[id] = { name: byId[+id] || "?", ...v };
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "item_activate_iv_sverka.json");
fs.writeFileSync(outPath, JSON.stringify(enriched, null, 2));
console.log("wrote", outPath, "items", Object.keys(enriched).length);
for (const id of Object.keys(enriched).sort((a, b) => +a - +b)) {
  const e = enriched[id];
  const bits = [];
  if (e.iv13 != null) bits.push("food+" + e.iv13);
  if (e.iv14 != null) bits.push("water+" + e.iv14);
  if (e.iv15 != null) bits.push("blood+" + e.iv15);
  if (e.iv16 != null) bits.push("temp+" + e.iv16);
  if (bits.length) console.log(id, e.name, bits.join(" "));
}
