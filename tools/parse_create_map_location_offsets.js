const fs = require("fs");
const path = require("path");

const body = JSON.parse(
  fs.readFileSync(
    path.join(__dirname, "..", "cpp", "docs_gen", "functions", "create_map_location_body.json"),
    "utf8"
  )
);

const strs = new Set();
const nums = new Set();
(function walk(n) {
  if (typeof n === "string") strs.add(n);
  else if (typeof n === "number") {
    if (n >= -800 && n <= 800 && Math.abs(n) > 2) nums.add(n);
  } else if (Array.isArray(n)) n.forEach(walk);
})(body);

const building = [...strs].filter((s) => s.startsWith("b_") || s.includes("Buildings") || s.includes("village"));
console.log("building-related strings:", building.sort().join("\n  "));

console.log("\nnums in [-800,800]:", [...nums].sort((a, b) => a - b).join(", "));

// Find CreateObject with Buildings_base and nearby position literals
const flat = JSON.stringify(body);
const idx = flat.indexOf("Buildings_base");
console.log("\nBuildings_base count:", (flat.match(/Buildings_base/g) || []).length);

// Walk for pattern [4,891] spawn with layout tag Buildings_base — collect IV/layout nums after
const spawns = [];
function findSpawns(evts, ctx) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes("Buildings_base")) {
      const nums = [];
      (function w(n) {
        if (typeof n === "number" && n >= -800 && n <= 800) nums.push(n);
        else if (Array.isArray(n)) n.forEach(w);
      })(e);
      spawns.push({ len: s.length, nums: [...new Set(nums)].sort((a, b) => a - b) });
    }
    for (const x of e) findSpawns(x, ctx);
  }
}
findSpawns(body);
spawns.sort((a, b) => a.len - b.len);
console.log("\nBuildings_base spawn blocks:", spawns.length);
spawns.slice(0, 8).forEach((s, i) => console.log(i, "nums:", s.nums.join(", ")));
