/** Extract On Function block for a given name (substantial body, exact trigger). */
const fs = require("fs");
const path = require("path");

const fnName = process.argv[2];
if (!fnName) {
  console.error("usage: node extract_fn_onfunction.js <Name>");
  process.exit(1);
}

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

const hits = [];
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (!s.includes('"' + fnName + '"')) {
      for (const x of e) walk(x);
      continue;
    }
    // On Function trigger uses action id 40 on obj180 with name at [1,[2,name]]
    const trig = `[2,"${fnName}"]`;
    if (s.includes("[180,40,") && s.includes(trig) && s.indexOf(trig) < 500) {
      hits.push({ len: s.length, e });
    }
    for (const x of e) walk(x);
  }
}
project[6].forEach((sh) => walk(sh[1]));
hits.sort((a, b) => a.len - b.len);
console.log("on-function hits", hits.length, "for", fnName);
hits.forEach((h, i) => console.log(i, h.len));

const substantial = hits.filter((h) => h.len > 200);
// Prefer smallest substantial exact OnFunction body (avoids whole event groups).
const best = substantial[0] || hits[hits.length - 1];
if (!best) process.exit(1);

const out = path.join(__dirname, "..", "cpp", "docs_gen", "functions", fnName + ".json");
fs.writeFileSync(out, JSON.stringify(best.e, null, 2));
console.log("wrote", out, "len", best.len);
