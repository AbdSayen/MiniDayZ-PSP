/**
 * Extract event group names and Function plugin call names from Game_events
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

const sheets = project[6];
const groups = [];
const functionNames = new Set();
const globalVarHints = new Set();

function walkEvents(evts, sheetName, depth) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;

    // Group event in C2: typically [-1, "GroupName", active, ...] or similar
    // Common formats:
    // [1, "Group name", 1, ...]  group
    // [0, conditions, actions, subevents]
    if (typeof e[1] === "string" && (e[0] === 1 || e[0] === -1 || e[0] === true)) {
      groups.push({ sheet: sheetName, depth, name: e[1], active: e[2] });
    }
    if (typeof e[0] === "string" && e[0].length > 1 && e[0].length < 80) {
      groups.push({ sheet: sheetName, depth, name: e[0], kind: "name_at_0" });
    }

    // Collect string literals that look like function names
    (function collect(node) {
      if (typeof node === "string") {
        if (/^[A-Za-z_][A-Za-z0-9_ ]{2,50}$/.test(node)) {
          if (/^(fn_|FN_|Func|On |Do |Update |Spawn |Init |Load |Save |Check |Get |Set )/i.test(node) ||
              /health|hunger|thirst|blood|stamina|damage|zombie|inventory|craft|weapon|bleed|infect|radiation|warmth|spawn|loot|bunker|vehicle|weather|death|reload|attack|move/i.test(node)) {
            functionNames.add(node);
          }
        }
      } else if (Array.isArray(node)) {
        node.forEach(collect);
      }
    })(e);

    // Recurse into nested arrays that look like event lists
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length > 0 && Array.isArray(e[i][0])) {
        walkEvents(e[i], sheetName, depth + 1);
      }
    }
  }
}

sheets.forEach((s) => {
  console.log("Walking sheet:", s[0]);
  walkEvents(s[1], s[0], 0);
});

console.log("\n=== EVENT GROUPS (" + groups.length + ") ===");
groups.forEach((g) => console.log("  [" + g.sheet + "] d" + g.depth + " " + g.name));

console.log("\n=== FUNCTION-LIKE / SYSTEM STRINGS (" + functionNames.size + ") ===");
[...functionNames].sort().forEach((n) => console.log("  " + n));

// Dump top-level structure of Game_events first events
const game = sheets.find((s) => s[0] === "Game_events");
if (game) {
  console.log("\n=== Game_events TOP-LEVEL STRUCTURE (first 40) ===");
  game[1].slice(0, 40).forEach((e, i) => {
    const preview = JSON.stringify(e).slice(0, 180);
    console.log(i + ": " + preview);
  });
}

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(
  path.join(outDir, "event_groups.json"),
  JSON.stringify({ groups, functionNames: [...functionNames].sort() }, null, 2)
);
