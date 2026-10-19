const fs = require("fs");
const path = require("path");
const project = JSON.parse(
  fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "")
).project;
const outDir = path.join("cpp", "docs_gen");

// Layout instances: [x,y,w,h,angle, typeIndex, name, ...]
const mapLayout = project[5].find((l) => l[0] === "Map" || l[0] === "map");
const lines = ["# map_element (type 501) layout instances on Map", ""];
if (mapLayout) {
  const inst = [];
  function walk(v, d) {
    if (!Array.isArray(v) || d > 15) return;
    if (v.length >= 8 && v[5] === 501) inst.push(v);
    for (const x of v) if (Array.isArray(x)) walk(x, d + 1);
  }
  walk(mapLayout, 0);
  lines.push("count=" + inst.length);
  for (const v of inst.slice(0, 30)) {
    lines.push(JSON.stringify(v).slice(0, 500));
  }
}

// create_map_element: parse Map_generation group from event_groups path
const mgPath = path.join(outDir, "groups", "Map_generation.json");
if (fs.existsSync(mgPath)) {
  const mg = JSON.parse(fs.readFileSync(mgPath, "utf8"));
  const pairs = new Map();
  for (const block of mg.blocks || mg.events || []) {
    const lit = block.literals || block;
    if (!Array.isArray(lit)) continue;
    const js = JSON.stringify(lit);
    if (!js.includes("create_map_element") && !js.includes("501")) continue;
    // pattern: [10, ivIndex], [7, tileExpr], [0, tileVal]
    for (let i = 0; i < lit.length - 4; i++) {
      if (lit[i] === 10 && typeof lit[i + 1] === "number") {
        const iv = lit[i + 1];
        // look ahead for [0, n] tile
        for (let j = i + 2; j < Math.min(i + 30, lit.length - 1); j++) {
          if (lit[j] === 0 && typeof lit[j + 1] === "number" && lit[j + 1] <= 20) {
            const tile = lit[j + 1];
            const key = `${iv}:${tile}`;
            pairs.set(key, (pairs.get(key) || 0) + 1);
          }
        }
      }
    }
  }
  lines.push("", "# IV index : tile value (from Map_generation.json literals)");
  for (const k of [...pairs.keys()].sort()) lines.push(k + " hits=" + pairs.get(k));
}

// Mag: scan Mag_reload_body for 909 IV17 numeric sets via plugin 10 actions
const magPath = path.join(outDir, "functions", "Mag_reload_body.json");
if (fs.existsSync(magPath)) {
  const body = JSON.parse(fs.readFileSync(magPath, "utf8"));
  const s = JSON.stringify(body);
  const caps = [...s.matchAll(/"IV17",(\d+)/g)].map((m) => +m[1]);
  lines.push("", "# Mag_reload_body IV17 literals", [...new Set(caps)].sort((a, b) => a - b).join(", "));
}

// player_weapons type 909 default IVs from object type
const ot909 = project[3][909];
if (ot909) {
  lines.push("", "# object type 909 snippet", JSON.stringify(ot909).slice(0, 2000));
}

// Items array col search for weapons 151-210 - project[3][536] might be array type
const arr536 = project[3][536];
lines.push("", "# type 536 exists", !!arr536);

// Scan all events for SetInstanceVar on 909 IV17 with weapon id in same tree
const magByWpn = {};
function walkEv(evts) {
  if (!Array.isArray(evts)) return;
  const js = JSON.stringify(evts);
  if (js.includes("909") && js.includes("IV17")) {
    const caps = [...js.matchAll(/"IV17",(\d+)/g)].map((m) => +m[1]);
    const wids = [...js.matchAll(/IV15.*?\[0,(\d{3})\]/g)].map((m) => +m[1]);
    for (const w of wids) {
      if (w < 151 || w > 210) continue;
      for (const c of caps) {
        if (c < 1 || c > 100) continue;
        magByWpn[w] = magByWpn[w] || {};
        magByWpn[w][c] = (magByWpn[w][c] || 0) + 1;
      }
    }
  }
  for (const e of evts) if (Array.isArray(e)) walkEv(e);
}
project[6].forEach((sh) => walkEv(sh[1]));
lines.push("", "# mag by weapon (909 IV17 in same event tree)");
for (const w of Object.keys(magByWpn).sort((a, b) => +a - +b)) {
  const top = Object.entries(magByWpn[w]).sort((a, b) => b[1] - a[1])[0];
  lines.push(`${w}: ${top[0]} (${top[1]} hits)`);
}

fs.writeFileSync(path.join(outDir, "map501_mag_decode.txt"), lines.join("\n"));
console.log("wrote map501_mag_decode.txt");
