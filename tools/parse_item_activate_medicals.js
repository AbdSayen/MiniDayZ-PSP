/**
 * Extended item_activate decode: timers (IV21 using-item), IV32/IV34/IV49, weapon IV6 (cleaning).
 */
const fs = require("fs");
const path = require("path");

const src = path.join(__dirname, "..", "cpp", "docs_gen", "functions", "item_activate_decoded.txt");
const lines = fs.readFileSync(src, "utf8").split(/\r?\n/);

const reParam = /^  IF obj180 fn\.cnd\.CompareParam \[\[0,0\],\[8,0\],(\d+)\]/;
const reWait = /Wait \[\[0,(\d+(?:\.\d+)?)\]\]/;
const reAdd = /obj172 spr\.act\.AddInstanceVar \["IV(\d+)",(-?\d+(?:\.\d+)?)\]/;
const reSub = /obj172 spr\.act\.SubInstanceVar \["IV(\d+)",(-?\d+(?:\.\d+)?)\]/;
const reSet = /obj172 spr\.act\.SetInstanceVar \["IV(\d+)",(-?\d+(?:\.\d+)?)\]/;
const reSetBool = /obj172 spr\.act\.SetInstanceVar \["IV(\d+)",1\]/;
const reClearBool = /obj172 spr\.act\.SetInstanceVar \["IV(\d+)",0\]/;

const blocks = {};
let cur = null;

for (const line of lines) {
  const pm = line.match(reParam);
  if (pm) {
    cur = +pm[1];
    if (!blocks[cur]) blocks[cur] = { waits: [], iv: {}, flags: {}, weapon_iv6: null, notes: [] };
    continue;
  }
  if (cur == null) continue;
  // next top-level item param ends block (same indent CompareParam)
  if (/^  IF obj180 fn\.cnd\.CompareParam/.test(line) && !line.includes(String(cur))) {
    // only when different id at end — handled by pm on next iteration
  }

  const wm = line.match(reWait);
  if (wm) blocks[cur].waits.push(parseFloat(wm[1]));

  for (const re of [reAdd, reSub, reSet]) {
    const m = line.match(re);
    if (m) {
      const iv = m[1];
      const v = parseFloat(m[2]);
      blocks[cur].iv["IV" + iv] = { op: re === reSub ? "sub" : re === reAdd ? "add" : "set", value: v };
    }
  }
  if (reSetBool.test(line)) {
    const m = line.match(reSetBool);
    blocks[cur].flags["IV" + m[1]] = true;
  }
  if (reClearBool.test(line)) {
    const m = line.match(reClearBool);
    blocks[cur].flags["IV" + m[1] + "_clear"] = true;
  }
  if (line.includes("obj909 spr.act.AddInstanceVar") && line.includes("IV6")) {
    const m = line.match(/AddInstanceVar \["IV6",(\d+)\]/);
    if (m) blocks[cur].weapon_iv6 = { add: +m[1] };
  }
}

const items = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "data", "items.json"), "utf8")
).items;
const byId = Object.fromEntries(items.map((it) => [it.id, it.name]));

const enriched = {};
for (const [id, b] of Object.entries(blocks)) {
  enriched[id] = { name: byId[+id] || "?", ...b };
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "item_activate_medicals_sverka.json");
fs.writeFileSync(outPath, JSON.stringify(enriched, null, 2));
console.log("wrote", outPath);

for (const id of ["19", "28", "29", "36", "45", "46"]) {
  const e = enriched[id];
  if (!e) {
    console.log(id, byId[+id] || "?", "NO CompareParam block in decode");
    continue;
  }
  console.log("\n", id, e.name);
  console.log("  waits", e.waits);
  console.log("  iv", JSON.stringify(e.iv));
  console.log("  flags", JSON.stringify(e.flags));
  if (e.weapon_iv6) console.log("  weapon_iv6", e.weapon_iv6);
}
