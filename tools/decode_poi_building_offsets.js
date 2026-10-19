/**
 * Extract POI Buildings_base spawn offsets from create_map_element (data.js).
 * Writes cpp/docs_gen/poi_building_offsets.json
 */
const fs = require("fs");
const path = require("path");

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

let bestStr = "";
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes("create_map_element") && s.includes("Buildings_base") && s.length > bestStr.length)
      bestStr = s;
    for (const x of e) walk(x);
  }
}
project[6].forEach((sh) => walk(sh[1]));
console.log("create_map_element body length", bestStr.length);

// Spawn: sys CreateObject → [501,73,... params include layer name + layout tag in [5,[2,"tag"]]
const spawns = [];
function walkSpawns(node) {
  if (!Array.isArray(node)) return;
  if (node[0] === 501 && node[1] === 73 && Array.isArray(node[5])) {
    const params = node[5];
    let tag = null;
    let layer = null;
    let typeId = null;
    for (const p of params) {
      if (!Array.isArray(p)) continue;
      if (p[0] === 4) typeId = p[1];
      if (p[0] === 5 && Array.isArray(p[1]) && p[1][1]) layer = p[1][1];
      if (p[0] === 7 && Array.isArray(p[1])) {
        if (p[1][0] === 0 && typeof p[1][1] === "number") tag = p[1][1];
        else tag = p[1];
      }
    }
    if (layer === "Buildings_base") {
      spawns.push({ layer, tag, typeId });
    }
  }
  for (const x of node) walkSpawns(x);
}
walkSpawns(JSON.parse(bestStr));
console.log("Buildings spawns parsed", spawns.length);

function flattenExpr(expr, out) {
  if (expr == null) return;
  if (typeof expr === "number") {
    if (Math.abs(expr) >= 10 && Math.abs(expr) <= 800) out.nums.push(expr);
    return;
  }
  if (!Array.isArray(expr)) return;
  if (typeof expr[0] === "number" && expr.length === 2 && typeof expr[1] === "number") {
    flattenExpr(expr[1], out);
    return;
  }
  for (const x of expr) flattenExpr(x, out);
  if (expr[0] === 23 && typeof expr[1] === "string") out.vars.add(expr[1]);
}

const byTag = {};
for (const s of spawns) {
  const tagKey =
    typeof s.tag === "number"
      ? "v" + s.tag
      : Array.isArray(s.tag)
        ? JSON.stringify(s.tag)
        : String(s.tag);
  const out = { nums: [], vars: new Set() };
  flattenExpr(s.xExpr, out);
  flattenExpr(s.yExpr, out);
  byTag[tagKey] = byTag[tagKey] || [];
  byTag[tagKey].push({
    typeId: s.typeId,
    nums: [...new Set(out.nums)].sort((a, b) => a - b),
    vars: [...out.vars],
  });
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "poi_building_offsets.json");
fs.writeFileSync(outPath, JSON.stringify({ spawnCount: spawns.length, byTag }, null, 2));
console.log("wrote", outPath);
console.log("tags", Object.keys(byTag).slice(0, 30).join(", "));
