/**
 * Extract nested event group names from C2 sheets.
 * Group format: [0, [true, "Name"], ...]
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function walk(evts, depth, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    // Group: [0, [true, "Name"], ...]
    if (e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true && typeof e[1][1] === "string") {
      out.push({ depth, name: e[1][1] });
    }
    // Subevents usually at index 5 (actions at 4, conditions at 3) — search all
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length > 0 && Array.isArray(e[i][0])) {
        walk(e[i], depth + (e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true ? 1 : 0), out);
      }
    }
  }
}

const all = {};
project[6].forEach((sheet) => {
  const out = [];
  walk(sheet[1], 0, out);
  all[sheet[0]] = out;
  console.log("=== " + sheet[0] + " (" + out.length + " groups) ===");
  out.forEach((g) => console.log("  " + "  ".repeat(g.depth) + g.name));
});

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, "group_tree.json"), JSON.stringify(all, null, 2));
