/**
 * Extract Construct Function plugin call names (readable entry points).
 */
const fs = require("fs");
const path = require("path");

const dataPath = process.argv[2] || path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

const names = new Set();
function walk(node) {
  if (typeof node === "string") {
    // Function calls often appear as plain strings in action params
    if (/^[A-Za-z_][A-Za-z0-9_]{2,48}$/.test(node)) {
      // filter common noise
      if (!/^(true|false|null|Default|Android|images|media)$/.test(node)) {
        names.add(node);
      }
    }
  } else if (Array.isArray(node)) {
    node.forEach(walk);
  }
}

project[6].forEach((sheet) => walk(sheet));

const list = [...names].sort();
const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, "function_like_strings.json"), JSON.stringify(list, null, 2));
console.log("count", list.length);
list.filter((n) =>
  /dmg|damage|spawn|loot|craft|reload|save|load|zed|wolf|player|bullet|inv|hunger|thirst|bleed|perk|quest|airdrop|vehicle|bunker|weather|day|night|heal|food|water/i.test(
    n
  )
).forEach((n) => console.log(n));
