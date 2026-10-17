const fs = require("fs");
const path = require("path");

const fnName = process.argv[2];
const maxLen = parseInt(process.argv[3] || "80000", 10);
const minLen = parseInt(process.argv[4] || "0", 10);
const namePos = parseInt(process.argv[5] || "250", 10);
const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    const pos = s.indexOf('"' + fnName + '"');
    if (pos >= 0 && pos < namePos && e[0] === 0 && s.length <= maxLen && s.length >= minLen) {
      out.push(e);
    }
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log("hits", hits.length);
hits.slice(0, 8).forEach((e, i) => console.log(i, JSON.stringify(e).length));
const pickIdx = process.argv[6] === "smallest" ? 0 : hits.length - 1;
const best = hits[pickIdx] || hits[0];
if (!best) process.exit(1);
const strs = new Set();
const nums = new Set();
(function w(x) {
  if (typeof x === "string") strs.add(x);
  else if (typeof x === "number") nums.add(x);
  else if (Array.isArray(x)) x.forEach(w);
})(best);
console.log("picked len", JSON.stringify(best).length);
console.log("strings:", [...strs].join(" | "));
console.log(
  "nums:",
  [...nums]
    .filter((x) => Math.abs(x) < 500)
    .sort((a, b) => a - b)
    .join(", ")
);
const outDir = path.join(__dirname, "..", "cpp", "docs_gen", "functions");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, fnName + "_body.json"), JSON.stringify(best, null, 2));
