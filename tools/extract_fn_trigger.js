/** Extract smallest event tree whose Function trigger matches name. */
const fs = require("fs");
const path = require("path");

const fnName = process.argv[2];
if (!fnName) {
  console.error("usage: node extract_fn_trigger.js <Name>");
  process.exit(1);
}

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

function walk(evts, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    const needle = '"' + fnName + '"';
    if (s.includes(needle) && s.length < 80000) {
      // trigger: obj180 fn.act.CallFunction with name near start
      if (s.indexOf(needle) < 400) out.push({ len: s.length, e });
    }
    for (const x of e) if (Array.isArray(x)) walk(x, out);
  }
}

const hits = [];
project[6].forEach((sh) => walk(sh[1], hits));
hits.sort((a, b) => a.len - b.len);
console.log("hits", hits.length, "for", fnName);
hits.slice(0, 8).forEach((h, i) => console.log(i, h.len));
// Prefer smallest tree that looks like a full Function trigger block (has sub-events).
const full = hits.filter((h) => h.len > 500 && h.e[0] === 0);
full.sort((a, b) => a.len - b.len);
const best = full[0] || hits[hits.length - 1];
if (best) {
  const outDir = path.join(__dirname, "..", "cpp", "docs_gen", "functions");
  fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(path.join(outDir, fnName + "_trigger.json"), JSON.stringify(best.e, null, 2));
  console.log("wrote", fnName + "_trigger.json", "len", best.len);
}
