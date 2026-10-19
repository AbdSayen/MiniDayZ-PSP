/**
 * Map create_map_element layout tag strings → Buildings_base spawn type ids + SetX/Y literals.
 */
const fs = require("fs");
const path = require("path");

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

let root = null;
function walkFind(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes("create_map_element") && s.includes("Buildings_base") && (!root || s.length > JSON.stringify(root).length))
      root = e;
    for (const x of e) walkFind(x);
  }
}
project[6].forEach((sh) => walkFind(sh[1]));

const layoutTags = new Set([
  "white", "brown2", "brown1", "brown3", "red", "red2", "brick", "city", "city2", "city3",
  "ga_brown", "ga_blue", "church", "firestation", "firestation2", "shtab", "barrack", "spawn1",
  "spawn2", "spawn3", "w1", "w2", "w3", "w4", "c1", "c2", "l1", "l2", "l3", "s1", "s2",
]);

const rows = [];
let ctxTag = null;

function numsFromExpr(expr, out) {
  if (expr == null) return;
  if (typeof expr === "number") {
    if (Math.abs(expr) >= 20 && Math.abs(expr) <= 600) out.push(expr);
    return;
  }
  if (!Array.isArray(expr)) return;
  for (const x of expr) numsFromExpr(x, out);
}

function walkNode(node) {
  if (!Array.isArray(node)) return;
  const flat = JSON.stringify(node);
  for (const tag of layoutTags) {
    if (flat.includes(`"${tag}"`)) ctxTag = tag;
  }
  if (node[0] === 501 && node[1] === 73 && Array.isArray(node[5])) {
    let layer = null;
    let variant = null;
    let typeId = null;
    for (const p of node[5]) {
      if (!Array.isArray(p)) continue;
      if (p[0] === 4) typeId = p[1];
      if (p[0] === 5 && p[1] && p[1][1]) layer = p[1][1];
      if (p[0] === 7 && p[1] && p[1][0] === 0) variant = p[1][1];
    }
    if (layer === "Buildings_base") {
      rows.push({ ctxTag, typeId, variant });
    }
  }
  if (node[0] === 501 && node[1] === 88 && Array.isArray(node[5])) {
    const nums = [];
    for (const p of node[5]) numsFromExpr(p, nums);
    if (nums.length && rows.length) {
      const last = rows[rows.length - 1];
      last.iv = last.iv || {};
      for (const p of node[5]) {
        if (Array.isArray(p) && p[0] === 10 && typeof p[1] === "number") {
          last.iv["IV" + p[1]] = numsFromExpr(p[7], []) || nums;
        }
      }
      last.nums = [...new Set(nums)].sort((a, b) => a - b);
    }
  }
  for (const x of node) walkNode(x);
}
walkNode(root);

const byTag = {};
for (const r of rows) {
  const k = r.ctxTag || "?";
  byTag[k] = byTag[k] || [];
  byTag[k].push({ typeId: r.typeId, variant: r.variant, nums: r.nums || [] });
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "poi_layout_tags.json");
fs.writeFileSync(outPath, JSON.stringify({ rowCount: rows.length, byTag }, null, 2));
console.log("rows", rows.length, "tags", Object.keys(byTag).join(", "));
console.log("wrote", outPath);
