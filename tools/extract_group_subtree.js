/**
 * Dump a named event group subtree from Game_events for reverse engineering.
 * Usage: node tools/extract_group_subtree.js "Player stats" [maxDepth]
 */
const fs = require("fs");
const path = require("path");

const groupName = process.argv[2] || "Player stats";
const maxDepth = parseInt(process.argv[3] || "4", 10);
const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");

function isGroup(e) {
  return e && e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true && typeof e[1][1] === "string";
}

function findGroup(evts, name) {
  if (!Array.isArray(evts)) return null;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (isGroup(e) && e[1][1] === name) return e;
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) {
        const f = findGroup(e[i], name);
        if (f) return f;
      }
    }
  }
  return null;
}

function summarize(e, depth) {
  if (depth > maxDepth) return "[...]";
  if (!Array.isArray(e)) return e;
  if (isGroup(e)) {
    const kids = [];
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) {
        for (const child of e[i]) kids.push(summarize(child, depth + 1));
      }
    }
    return { group: e[1][1], children: kids.slice(0, 80) };
  }
  // Ordinary event: collect string/number literals
  const literals = [];
  (function walk(n, d) {
    if (d > 12) return;
    if (typeof n === "string" || typeof n === "number" || typeof n === "boolean") {
      literals.push(n);
    } else if (Array.isArray(n)) n.forEach((x) => walk(x, d + 1));
  })(e, 0);
  return { event: true, literals: literals.slice(0, 60) };
}

const g = findGroup(game[1], groupName);
if (!g) {
  console.error("Group not found:", groupName);
  process.exit(1);
}

const out = summarize(g, 0);
const outDir = path.join(__dirname, "..", "cpp", "docs_gen", "groups");
fs.mkdirSync(outDir, { recursive: true });
const safe = groupName.replace(/[^a-zA-Z0-9_-]+/g, "_");
fs.writeFileSync(path.join(outDir, safe + ".json"), JSON.stringify(out, null, 2));

// Also flatten interesting number/string pairs
function flattenLiterals(node, acc) {
  if (!node) return;
  if (node.literals) acc.push(...node.literals);
  if (node.children) node.children.forEach((c) => flattenLiterals(c, acc));
}
const lit = [];
flattenLiterals(out, lit);
const nums = lit.filter((x) => typeof x === "number");
const strs = lit.filter((x) => typeof x === "string" && x.length > 1 && x.length < 60);

console.log("Group:", groupName);
console.log("Unique strings:", [...new Set(strs)].slice(0, 100).join(" | "));
console.log("Number histogram (top):");
const hist = {};
nums.forEach((n) => {
  const k = String(n);
  hist[k] = (hist[k] || 0) + 1;
});
Object.entries(hist)
  .sort((a, b) => b[1] - a[1])
  .slice(0, 40)
  .forEach(([k, v]) => console.log(" ", v, k));
