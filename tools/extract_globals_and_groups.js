/**
 * Separate C2 globals vs real event groups; dump group tree.
 * C2 sheet entry kinds (from c2runtime):
 *   0 = ordinary event (sometimes omitted / different)
 *   1 = global/local variable
 *   2 = include sheet
 *   3 = event group? — verify empirically
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const entries = game[1];

const globals = [];
const includes = [];
const others = [];
const kindCounts = {};

entries.forEach((e, i) => {
  if (!Array.isArray(e)) return;
  const k = e[0];
  kindCounts[k] = (kindCounts[k] || 0) + 1;
  if (k === 1 && typeof e[1] === "string") {
    const type = e[2]; // 0 number, 1 text
    const init = e[3];
    globals.push({ name: e[1], type: type === 1 ? "text" : "number", init });
  } else if (k === 2) {
    includes.push(e[1]);
  } else {
    others.push({ i, kind: k, preview: JSON.stringify(e).slice(0, 200) });
  }
});

console.log("Kind counts:", JSON.stringify(kindCounts));
console.log("Globals:", globals.length);
console.log("Includes:", includes);
console.log("Other top-level:", others.length);
others.slice(0, 60).forEach((o) => console.log(o.i, "kind="+o.kind, o.preview));

// Walk for group blocks: in many C2 exports a group is an event with
// conditions empty and a string "group" flag — search for pattern [false,"Name"
// or events where e[1] is string and e[0] is false/0
function findGroups(evts, depth, pathAcc, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    // Group format observed in some exports: [false, "GroupName", 1, ...] 
    // Or: nested under parent with name in trigger
    if (e[0] === false && typeof e[1] === "string") {
      out.push({ depth, path: pathAcc, name: e[1], active: e[2] });
      // subgroups often in e[4] or last array
    }
    if (typeof e[1] === "string" && e[0] === 0) {
      out.push({ depth, path: pathAcc, name: e[1], note: "kind0" });
    }
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) {
        const nameHint = typeof e[1] === "string" ? e[1] : pathAcc;
        findGroups(e[i], depth + 1, nameHint, out);
      }
    }
  }
}

const groups = [];
findGroups(entries, 0, "root", groups);
console.log("\nGroups found:", groups.length);
groups.slice(0, 150).forEach((g) =>
  console.log("  d" + g.depth, g.name, g.active !== undefined ? "active="+g.active : "", g.note || "")
);

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(
  path.join(outDir, "globals.json"),
  JSON.stringify({ globals, includes, kindCounts, otherCount: others.length }, null, 2)
);

// Emit C++ header with globals as constants where init is numeric
let hpp = "#pragma once\n// Auto-extracted from original_docs/data.js Game_events globals\n";
hpp += "// Version Mini DAYZ 1.4.1 — DO NOT hand-edit balance without verifying against source\n\n";
hpp += "namespace mdaz::balance {\n\n";
globals.forEach((g) => {
  if (g.type === "number" && typeof g.init === "number") {
    const cname = g.name.replace(/[^A-Za-z0-9_]/g, "_");
    hpp += "constexpr double " + cname + " = " + g.init + ";\n";
  }
});
hpp += "\n} // namespace mdaz::balance\n";
fs.writeFileSync(path.join(outDir, "extracted_globals.hpp"), hpp);
console.log("\nWrote globals.json and extracted_globals.hpp");
