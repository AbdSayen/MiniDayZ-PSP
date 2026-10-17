const fs = require("fs");
const project = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project;
const hits = [];
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes("deathtime") || s.includes("bot_tutor")) hits.push(s);
    for (const x of e) if (Array.isArray(x)) walk(x);
  }
}
project[6].forEach((sh) => walk(sh[1]));
hits.sort((a, b) => a.length - b.length);
console.log("hits", hits.length);
for (const h of hits.slice(0, 5)) {
  const strs = [...h.matchAll(/"([^"]{2,60})"/g)].map((m) => m[1]);
  console.log("len", h.length, strs.filter((x) => !/^\d+$/.test(x)).slice(0, 25).join(" | "));
}
