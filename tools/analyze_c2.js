/**
 * Analyze Construct 2 Mini DAYZ export (original_docs/data.js)
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const raw = fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "");
const project = JSON.parse(raw).project;

const out = [];
function log(s) {
  out.push(s);
  console.log(s);
}

log("=== Mini DAYZ Construct 2 Analysis ===");
log("Source: " + dataPath);
log("Version field: " + project[16]);
log("Title: " + project[26]);
log("Resolution: " + project[10] + "x" + project[11]);
log("First layout: " + project[1]);
log("");

// Layouts
log("=== LAYOUTS ===");
project[5].forEach((l, i) => {
  log(`  [${i}] ${l[0]}  size=${l[1]}x${l[2]}`);
});
log("");

// Event sheets
log("=== EVENT SHEETS ===");
project[6].forEach((s, i) => {
  const top = Array.isArray(s[1]) ? s[1].length : 0;
  log(`  [${i}] ${s[0]}  top-level groups/events: ${top}`);
});
log("");

// Object types: recover names from image paths
const objs = project[3];
log("=== OBJECT TYPES: " + objs.length + " ===");

const pluginCounts = {};
const prefixes = {};
const objects = [];

objs.forEach((o, idx) => {
  const id = o[0];
  const plugin = o[1];
  pluginCounts[plugin] = (pluginCounts[plugin] || 0) + 1;

  const imgs = [];
  (function walk(a) {
    if (!a) return;
    if (typeof a === "string" && a.indexOf("images/") === 0) imgs.push(a);
    else if (Array.isArray(a)) a.forEach(walk);
  })(o);

  const pref = [];
  imgs.forEach((p) => {
    const m = p.match(/^images\/([^-./]+)/);
    if (m) pref.push(m[1]);
  });
  const uniqPref = [...new Set(pref)];
  const primary = uniqPref[0] || ("plugin" + plugin + "_" + id);

  if (!prefixes[primary]) prefixes[primary] = [];
  prefixes[primary].push(id);

  objects.push({
    idx,
    id,
    plugin,
    prefixes: uniqPref,
    ivSids: Array.isArray(o[3]) ? o[3] : [],
    imageCount: imgs.length,
  });
});

log("Plugin usage:");
Object.keys(pluginCounts)
  .sort((a, b) => pluginCounts[b] - pluginCounts[a])
  .forEach((p) => log("  plugin " + p + ": " + pluginCounts[p]));

log("");
log("Unique image-name prefixes (" + Object.keys(prefixes).length + "):");
Object.keys(prefixes)
  .sort()
  .forEach((p) => log("  " + p + " (" + prefixes[p].length + ")"));

// Categorize by name patterns
const categories = {
  player: [],
  zombie: [],
  animal: [],
  weapon: [],
  item: [],
  gui: [],
  map: [],
  vehicle: [],
  building: [],
  nature: [],
  effect: [],
  other: [],
};

function categorize(name) {
  const n = name.toLowerCase();
  if (/player|survivor|char_/.test(n)) return "player";
  if (/zed|zombie|infected/.test(n)) return "zombie";
  if (/wolf|deer|cow|pig|animal|bear|crow|dog/.test(n)) return "animal";
  if (/gun|rifle|pistol|ammo|bullet|melee|axe|knife|bow|weapon|ak|m4|mosin|shotgun/.test(n))
    return "weapon";
  if (/gui_|ui_|btn_|button|panel|slot|inv|hud|icon_/.test(n)) return "gui";
  if (/map_|tile|chunk|minimap|fog/.test(n)) return "map";
  if (/car|uaz|volga|vehicle|truck|bike/.test(n)) return "vehicle";
  if (/house|building|door|wall|fence|tent|bunker|barn|shed/.test(n)) return "building";
  if (/tree|bush|grass|rock|water|snow|ground|forest/.test(n)) return "nature";
  if (/fx_|effect|particle|blood|smoke|fire|explosion/.test(n)) return "effect";
  if (/item_|loot|food|drink|med|bandage|backpack|cloth|gear/.test(n)) return "item";
  return "other";
}

Object.keys(prefixes).forEach((p) => {
  categories[categorize(p)].push(p);
});

log("");
log("=== CATEGORIZED PREFIXES ===");
Object.keys(categories).forEach((c) => {
  log(c + " (" + categories[c].length + "): " + categories[c].sort().join(", "));
});

// Extract readable strings from event sheets (game logic clues)
log("");
log("=== STRINGS IN EVENT SHEETS (unique, filtered) ===");
const sheetStr = JSON.stringify(project[6]);
const re = /"([^"\\]{3,60})"/g;
const counts = {};
let m;
while ((m = re.exec(sheetStr))) {
  const s = m[1];
  // skip pure numbers, image paths, mostly noise
  if (/^[\d.]+$/.test(s)) continue;
  if (s.indexOf("images/") === 0) continue;
  if (s.indexOf("media/") === 0) continue;
  if (/^[a-f0-9]{10,}$/i.test(s)) continue;
  counts[s] = (counts[s] || 0) + 1;
}

const sorted = Object.entries(counts).sort((a, b) => b[1] - a[1]);
const interesting = sorted.filter(([s]) => {
  return (
    /health|hunger|thirst|blood|stamina|fatigue|damage|zombie|player|inventory|craft|weapon|ammo|bleed|infect|radiation|warmth|temp|save|load|spawn|loot|quest|bunker|vehicle|car|wolf|deer|map|day|night|weather|rain|snow|fire|food|water|bandage|heal|kill|death|menu|tutorial/i.test(
      s
    ) || /^[A-Z][a-zA-Z0-9_ ]+$/.test(s)
  );
});

interesting.slice(0, 400).forEach(([s, c]) => log(c + "\t" + s));

// Audio list summary
log("");
log("=== AUDIO FILES: " + project[7].length + " ===");
const audioNames = project[7].map((a) => a[0]);
const audioUnique = [...new Set(audioNames.map((n) => n.replace(/\.(ogg|m4a)$/i, "")))];
log("Unique base names: " + audioUnique.length);
audioUnique.sort().forEach((n) => log("  " + n));

// Localization sample
const locPath = path.join(path.dirname(dataPath), "l_eng_items.xml");
if (fs.existsSync(locPath)) {
  log("");
  log("=== LOCALIZATION SAMPLE (l_eng_items.xml) ===");
  const loc = fs.readFileSync(locPath, "utf8");
  const items = [...loc.matchAll(/name="([^"]+)"/g)].map((x) => x[1]);
  log("Item locale keys: " + items.length);
  items.slice(0, 80).forEach((n) => log("  " + n));
}

// Families
log("");
log("=== FAMILIES: " + project[4].length + " ===");
project[4].forEach((f, i) => {
  log("  family[" + i + "] members=" + (f.length - 0) + " first=" + f[0]);
});

// Write JSON summary for later use
const summary = {
  version: project[16],
  title: project[26],
  resolution: [project[10], project[11]],
  layouts: project[5].map((l) => ({ name: l[0], w: l[1], h: l[2] })),
  eventSheets: project[6].map((s) => ({ name: s[0], topEvents: s[1].length })),
  objectCount: objs.length,
  pluginCounts,
  prefixes: Object.keys(prefixes).sort(),
  categories,
  objects,
  audioUnique: audioUnique.sort(),
};

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, "c2_analysis.json"), JSON.stringify(summary, null, 2));
fs.writeFileSync(path.join(outDir, "c2_analysis.txt"), out.join("\n"));
log("");
log("Wrote cpp/docs_gen/c2_analysis.json and c2_analysis.txt");
