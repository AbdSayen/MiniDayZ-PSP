/**
 * Decode Death_guide death flow, Save_loot/Load_loot CSV fields, mapgen calls.
 * Usage: node tools/decode_death_loot_mapgen.js [all|death|loot|mapgen]
 */
const fs = require("fs");
const path = require("path");

const root = path.join(__dirname, "..");
const dataPath = path.join(root, "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const outDir = path.join(root, "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });

function walk(evts, pred, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (pred(e)) out.push(e);
    for (const x of e) if (Array.isArray(x)) walk(x, pred, out);
  }
}

function collectStrings(n, set) {
  if (typeof n === "string") set.add(n);
  else if (Array.isArray(n)) n.forEach((x) => collectStrings(x, set));
}

function decodeDeathGuide() {
  const hits = [];
  project[6].forEach((sh) =>
    walk(sh[1], (e) => e[0] === 0 && JSON.stringify(e).includes('"Death_guide"'), hits)
  );
  hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const big = hits[hits.length - 1];
  const lines = ["# Death_guide group (largest sheet hit)", "len " + JSON.stringify(big).length, ""];

  const deathLines = [];
  function scan(e, ctx) {
    if (!Array.isArray(e)) return;
    const js = JSON.stringify(e);
    if (js.includes('"Deadscreen"') || js.includes('"tutor_death"') || js.includes('"deathtime"')) {
      const strs = new Set();
      collectStrings(e, strs);
      deathLines.push(
        "ctx=" +
          ctx +
          " | " +
          [...strs].filter((s) => s.length > 0 && s.length < 80).join(" | ")
      );
    }
    let i = 0;
    for (const x of e) {
      if (Array.isArray(x)) scan(x, ctx + "/" + i);
      i++;
    }
  }
  scan(big, "root");

  lines.push("## Deadscreen / tutor_death subtrees (" + deathLines.length + ")");
  lines.push(...deathLines.slice(0, 80));

  // On-function Death_guide (small)
  const onHits = [];
  project[6].forEach((sh) =>
    walk(
      sh[1],
      (e) =>
        Array.isArray(e) &&
        e.length >= 7 &&
        e[0] === 0 &&
        Array.isArray(e[1]) &&
        e[1][0] === true &&
        e[1][1] === "Death_guide",
      onHits
    )
  );
  lines.push("", "## On function Death_guide", JSON.stringify(onHits[0], null, 2));

  fs.writeFileSync(path.join(outDir, "death_guide_decode.txt"), lines.join("\n").slice(0, 200000));
  console.log("wrote death_guide_decode.txt", deathLines.length, "subtrees");
}

function decodeSaveLoot() {
  const hits = [];
  project[6].forEach((sh) =>
    walk(sh[1], (e) => e[0] === 0 && JSON.stringify(e).includes('"Save_loot"'), hits)
  );
  hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const fn = hits.find((h) => {
    const s = JSON.stringify(h);
    return s.includes('"Save_loot"') && s.includes("Difficulty") && s.length < 20000;
  });
  if (!fn) {
    console.log("Save_loot root not found");
    return;
  }

  const lines = ["# Save_loot function body decode", "len " + JSON.stringify(fn).length, ""];

  // Find all obj172 IV reads in concat (21,172,false,null,N)
  const ivs = new Set();
  (function walkIv(n) {
    if (!Array.isArray(n)) return;
    if (
      n.length >= 5 &&
      n[0] === 21 &&
      n[1] === 172 &&
      typeof n[4] === "number"
    )
      ivs.add(n[4]);
    n.forEach(walkIv);
  })(fn);
  lines.push("obj172 IV indices in CSV concat: " + [...ivs].sort((a, b) => a - b).join(", "));

  // obj909 / items_on_ground references
  const strs = new Set();
  collectStrings(fn, strs);
  lines.push("", "strings:", [...strs].sort().join("\n"));

  // Load_loot token indices (from split at comma positions 6,7,8,9...)
  const loadHits = [];
  project[6].forEach((sh) =>
    walk(sh[1], (e) => e[0] === 0 && JSON.stringify(e).includes('"Load_loot"'), loadHits)
  );
  loadHits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const loadFn = loadHits.find((h) => JSON.stringify(h).includes('"Load_loot"') && h.length < 15000);
  const loadIv = new Set();
  if (loadFn) {
    (function walkIv(n) {
      if (!Array.isArray(n)) return;
      if (n.length >= 3 && n[0] === 0 && typeof n[1] === "number" && n[1] >= 6 && n[1] <= 40)
        loadIv.add(n[1]);
      n.forEach(walkIv);
    })(loadFn);
    lines.push("", "# Load_loot CSV field indices (token positions): " + [...loadIv].sort((a, b) => a - b).join(", "));
  }

  fs.writeFileSync(path.join(outDir, "save_loot_decode.txt"), lines.join("\n"));
  console.log("wrote save_loot_decode.txt ivs", [...ivs].join(","));
}

function decodeMapgen() {
  const hits = [];
  project[6].forEach((sh) =>
    walk(
      sh[1],
      (e) => e[0] === 0 && JSON.stringify(e).includes('"generate_mapgen_toelements"'),
      hits
    )
  );
  hits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const pick = hits.find((h) => {
    const l = JSON.stringify(h).length;
    return l > 300 && l < 5000;
  });
  const lines = ["# generate_mapgen_toelements compact hit", ""];
  if (pick) lines.push(JSON.stringify(pick, null, 2));

  // create_map_element — find CallFunction with params
  const ceHits = [];
  project[6].forEach((sh) =>
    walk(sh[1], (e) => e[0] === 0 && JSON.stringify(e).includes('"create_map_element"'), ceHits)
  );
  ceHits.sort((a, b) => JSON.stringify(a).length - JSON.stringify(b).length);
  const ce = ceHits.find((h) => JSON.stringify(h).includes("create_map_element") && JSON.stringify(h).length < 3000);
  lines.push("", "# create_map_element sample", ce ? JSON.stringify(ce, null, 2) : "none");

  // Prop/plant strings in generate_mapgen window
  const raw = fs.readFileSync(dataPath, "utf8");
  const idx = raw.indexOf('"generate_mapgen_toelements"');
  const win = raw.slice(idx, idx + 120000);
  const propStr = [...win.matchAll(/"(pine|tree|b_[a-z_]+|prop_[a-z0-9_]+|bush|plant)[^"]{0,40}"/gi)].map((m) => m[0]);
  const freq = {};
  for (const p of propStr) freq[p] = (freq[p] || 0) + 1;
  lines.push("", "# prop strings near generate_mapgen_toelements (top 40)");
  lines.push(
    Object.entries(freq)
      .sort((a, b) => b[1] - a[1])
      .slice(0, 40)
      .map(([k, v]) => v + "\t" + k)
      .join("\n")
  );

  fs.writeFileSync(path.join(outDir, "mapgen_props_decode.txt"), lines.join("\n").slice(0, 400000));
  console.log("wrote mapgen_props_decode.txt props", Object.keys(freq).length);
}

const mode = process.argv[2] || "all";
if (mode === "all" || mode === "death") decodeDeathGuide();
if (mode === "all" || mode === "loot") decodeSaveLoot();
if (mode === "all" || mode === "mapgen") decodeMapgen();
