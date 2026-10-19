/**
 * Find Construct Function "On function" bodies by name and dump literals.
 * Usage: node tools/extract_function_body.js Update_Stats
 */
const fs = require("fs");
const path = require("path");

const fnName = process.argv[2] || "Update_Stats";
const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function walkFind(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const flat = JSON.stringify(e);
    // On Function typically includes the function name string and Function plugin type
    if (flat.indexOf('"' + fnName + '"') >= 0 && flat.length < 500000) {
      // Prefer events that look like On Function triggers (contain both name and short)
      out.push(e);
    }
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) {
        walkFind(e[i], out);
      }
    }
  }
}

const hits = [];
project[6].forEach((sheet) => walkFind(sheet[1], hits));

// Rank: prefer events where fnName appears early / as On function
function score(e) {
  const s = JSON.stringify(e);
  let sc = 0;
  if (s.indexOf('"' + fnName + '"') < 120) sc += 10;
  if (s.includes('"On function"') || s.match(/,\s*40,/)) sc += 5; // common Function action/cond id area
  sc += Math.min(20, Math.floor(s.length / 500));
  return sc;
}

hits.sort((a, b) => score(b) - score(a));
console.log("hits", hits.length, "for", fnName);

const top = hits.slice(0, 3);
top.forEach((e, idx) => {
  const lit = [];
  (function w(n, d) {
    if (d > 20) return;
    if (typeof n === "string" || typeof n === "number" || typeof n === "boolean") lit.push(n);
    else if (Array.isArray(n)) n.forEach((x) => w(x, d + 1));
  })(e, 0);
  const strs = lit.filter((x) => typeof x === "string");
  const nums = lit.filter((x) => typeof x === "number" && Math.abs(x) < 100000);
  console.log("\n=== HIT", idx, "len", JSON.stringify(e).length, "===");
  console.log("strings:", [...new Set(strs)].slice(0, 80).join(" | "));
  console.log(
    "nums:",
    [...new Set(nums)]
      .filter((n) => n !== 0 && n !== -1)
      .sort((a, b) => a - b)
      .slice(0, 60)
      .join(", ")
  );
});

const outDir = path.join(__dirname, "..", "cpp", "docs_gen", "functions");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(
  path.join(outDir, fnName.replace(/[^a-zA-Z0-9_]/g, "_") + ".json"),
  JSON.stringify(top[0] || null, null, 2).slice(0, 500000)
);
