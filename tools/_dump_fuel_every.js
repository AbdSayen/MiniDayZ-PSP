const fs = require("fs");
const project = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project;

function walk(evts, pred, out) {
  if (!Array.isArray(evts)) return;
  if (pred(JSON.stringify(evts), evts)) out.push(evts);
  for (const e of evts) if (Array.isArray(e)) walk(e, pred, out);
}

const out = [];
project[6].forEach((sh) => {
  walk(
    sh[1],
    (s) =>
      s.includes("Drivable vehicles") &&
      s.includes('"fuel"') &&
      (s.includes("0.1") || s.includes("Every")),
    out
  );
});
console.log("count", out.length);
for (const e of out) {
  console.log(JSON.stringify(e, null, 2).slice(0, 8000));
}
