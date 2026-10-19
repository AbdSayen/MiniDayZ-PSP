/**
 * Proven mag capacity: find events where player_weapons IV17 is compared to literal
 * in same subtree as weapon item id (IV15 on 909 or [0,weaponId]).
 */
const fs = require("fs");
const path = require("path");

const s = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");
const project = JSON.parse(s.replace(/^\uFEFF/, "")).project;

const results = {};

function walk(evts) {
  if (!Array.isArray(evts)) return;
  const js = JSON.stringify(evts);
  if (!js.includes("909") || !js.includes(",17")) return;

  const caps = [];
  // CompareInstanceVar IV17 == literal: pattern ,17 ... [8,0],N
  for (const m of js.matchAll(/909,false,null,17[\s\S]{0,120}?\[8,0\],(\d+)/g)) {
    caps.push(+m[1]);
  }
  for (const m of js.matchAll(/"IV17",\[8,0\],(\d+)/g)) {
    caps.push(+m[1]);
  }
  if (!caps.length) {
    for (const x of evts) if (Array.isArray(x)) walk(x);
    return;
  }

  const wids = new Set();
  for (const m of js.matchAll(/909,false,null,15[\s\S]{0,80}?\[8,0\],(\d+)/g)) wids.add(+m[1]);
  for (const m of js.matchAll(/IV15",\[8,0\],(\d+)/g)) wids.add(+m[1]);
  for (const m of js.matchAll(/\[0,(1[5-9]\d|2[0-1]\d)\]/g)) wids.add(+m[1]);

  for (const w of wids) {
    if (w < 151 || w > 210) continue;
    for (const c of caps) {
      if (c < 1 || c > 100) continue;
      if (!results[w]) results[w] = {};
      results[w][c] = (results[w][c] || 0) + 1;
    }
  }
  for (const x of evts) if (Array.isArray(x)) walk(x);
}

project[6].forEach((sh) => walk(sh[1]));

const lines = ["# mag_capacity_proof — player_weapons IV17 compare literals co-located with weapon id", ""];
for (const w of Object.keys(results).sort((a, b) => +a - +b)) {
  const top = Object.entries(results[w]).sort((a, b) => b[1] - a[1])[0];
  lines.push(`${w}: mag=${top[0]} (hits ${top[1]}) all=${JSON.stringify(results[w])}`);
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "mag_capacity_proof.txt");
fs.writeFileSync(out, lines.join("\n"));
console.log("weapons", Object.keys(results).length);
console.log(lines.slice(0, 25).join("\n"));
