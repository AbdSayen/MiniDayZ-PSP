const fs = require("fs");
const path = require("path");

const body = JSON.parse(
  fs.readFileSync(
    path.join(__dirname, "..", "cpp", "docs_gen", "functions", "create_map_location_body.json"),
    "utf8"
  )
);

// C2 spawn: [501,73,... [[4,typeId],[5,[2,"layout"]],[7,[0,variant]]]]
const results = [];
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (e[0] === 0 && Array.isArray(e[6])) {
      for (const act of e[6]) {
        if (!Array.isArray(act) || act[0] !== 501 || act[1] !== 73) continue;
        const params = act[8];
        if (!Array.isArray(params)) continue;
        let layout = null;
        let variant = null;
        let typeId = null;
        for (const p of params) {
          if (!Array.isArray(p)) continue;
          if (p[0] === 4) typeId = p[1];
          if (p[0] === 5 && Array.isArray(p[1]) && p[1][1]) layout = p[1][1];
          if (p[0] === 7 && Array.isArray(p[1]) && p[1][0] === 0) variant = p[1][1];
        }
        if (layout && String(layout).includes("b_")) {
          results.push({ layout, variant, typeId });
        }
      }
    }
    for (const x of e) walk(x);
  }
}
walk(body);

const byLayout = {};
for (const r of results) {
  byLayout[r.layout] = byLayout[r.layout] || new Set();
  byLayout[r.layout].add(r.variant);
}
console.log("b_ layout spawns:", results.length);
Object.keys(byLayout)
  .sort()
  .forEach((k) => console.log(k, "variants:", [...byLayout[k]].sort((a, b) => a - b).join(", ")));

// SetPosition / offset patterns: look for [0,[4,[20,507,65... with numeric offsets
const offsets = [];
function walkOff(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes('"SetX"') || (s.includes("507") && s.includes("-300"))) {
      const nums = [];
      (function w(n) {
        if (typeof n === "number" && Math.abs(n) >= 50 && Math.abs(n) <= 500) nums.push(n);
        else if (Array.isArray(n)) n.forEach(w);
      })(e);
      if (nums.length) offsets.push([...new Set(nums)].sort((a, b) => a - b));
    }
    for (const x of e) walk(x);
  }
}
walkOff(body);
console.log("\noffset num sets (sample):", offsets.slice(0, 15).map((o) => o.join(",")).join("\n"));
