const fs = require("fs");
const path = require("path");

const must = [];
const mustNot = [];
for (const arg of process.argv.slice(2)) {
  if (arg.startsWith("!")) mustNot.push(arg.slice(1));
  else must.push(arg);
}
const targetLen = parseInt(process.env.TARGET_LEN || "0", 10);
const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (e[0] !== 0) {
      for (const x of e) if (Array.isArray(x)) walk(x, out);
      continue;
    }
    if (!must.every((m) => s.includes('"' + m + '"'))) {
      for (const x of e) if (Array.isArray(x)) walk(x, out);
      continue;
    }
    if (mustNot.some((m) => s.includes('"' + m + '"'))) {
      for (const x of e) if (Array.isArray(x)) walk(x, out);
      continue;
    }
    out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
console.log("hits", hits.length, "must", must.join("+"), "not", mustNot.join(","));
let best = hits[0];
if (targetLen) {
  best = hits.reduce((a, b) => {
    const la = JSON.stringify(a).length;
    const lb = JSON.stringify(b).length;
    return Math.abs(la - targetLen) < Math.abs(lb - targetLen) ? a : b;
  }, hits[0]);
}
if (!best) process.exit(1);
console.log("len", JSON.stringify(best).length);
const strs = new Set();
(function w(x) {
  if (typeof x === "string") strs.add(x);
  else if (Array.isArray(x)) x.forEach(w);
})(best);
console.log([...strs].join(" | "));
const outName = (must[0] || "event") + "_impl.json";
fs.writeFileSync(
  path.join(__dirname, "..", "cpp", "docs_gen", "functions", outName),
  JSON.stringify(best, null, 2)
);
