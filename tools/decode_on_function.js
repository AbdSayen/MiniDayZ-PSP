/**
 * Dump Construct "On function" event trees for a given function name.
 * Usage: node tools/decode_on_function.js Check_ammo_inventory
 */
const fs = require("fs");
const path = require("path");

const fnName = process.argv[2];
if (!fnName) {
  console.error("usage: node decode_on_function.js <FunctionName>");
  process.exit(1);
}

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function isOnFunctionRoot(e) {
  return (
    Array.isArray(e) &&
    e.length >= 7 &&
    e[0] === 0 &&
    Array.isArray(e[1]) &&
    e[1][0] === true &&
    e[1][1] === fnName
  );
}

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (isOnFunctionRoot(e)) out.push(e);
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i])) walk(e[i], out);
    }
  }
}

const hits = [];
project[6].forEach((sheet) => walk(sheet[1], hits));
hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);

console.log("on-function roots", hits.length, "for", fnName);
hits.slice(0, 8).forEach((e, i) => {
  console.log(i, "len", JSON.stringify(e).length);
});

const outDir = path.join(__dirname, "..", "cpp", "docs_gen", "functions");
fs.mkdirSync(outDir, { recursive: true });
const best = hits[0] || null;
const outPath = path.join(outDir, fnName + "_decoded.txt");

function collectLiterals(n, strs, nums) {
  if (typeof n === "string") strs.add(n);
  else if (typeof n === "number" && Number.isFinite(n)) nums.add(n);
  else if (Array.isArray(n)) n.forEach((x) => collectLiterals(x, strs, nums));
}

if (!best) {
  fs.writeFileSync(outPath, "no On function root hit\n");
  process.exit(0);
}

const strs = new Set();
const nums = new Set();
collectLiterals(best, strs, nums);
const lines = [
  "len " + JSON.stringify(best).length,
  "strings: " + [...strs].join(" | "),
  "nums: " +
    [...nums]
      .filter((n) => Math.abs(n) < 10000)
      .sort((a, b) => a - b)
      .join(", "),
  "",
  "json:",
  JSON.stringify(best, null, 2),
];
fs.writeFileSync(outPath, lines.join("\n").slice(0, 400000));
fs.writeFileSync(
  path.join(outDir, fnName + "_on.json"),
  JSON.stringify(best, null, 2).slice(0, 500000)
);
console.log("wrote", outPath);
