const fs = require("fs");
const path = require("path");

const fnName = "create_map_location";
const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

const hits = [];
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    const trig = `[2,"${fnName}"]`;
    if (s.includes(trig) && s.includes("[180,40,") && s.indexOf(trig) < 500) {
      hits.push({ len: s.length, e });
    }
    for (const x of e) walk(x);
  }
}
project[6].forEach((sh) => walk(sh[1]));
hits.sort((a, b) => b.len - a.len);
console.log(
  "hits",
  hits.map((h) => h.len).join(", ")
);
if (!hits[0]) process.exit(1);
const out = path.join(__dirname, "..", "cpp", "docs_gen", "functions", "create_map_location_body.json");
fs.writeFileSync(out, JSON.stringify(hits[0].e));
console.log("wrote", out);
