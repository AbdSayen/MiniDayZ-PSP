/**
 * Probe layout object IV patterns near melee weapon name strings.
 */
const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const names = [
  "fireaxe", "crowbar", "shovel", "sword", "hatchet", "pipewrench", "bat",
  "Fist", "sledgehammer", "pickaxe", "pitchfork", "splitaxe", "knife_hunter"
];

for (const n of names) {
  // Pattern from melee_damage_hints: [[10],[0],[0],[400],["name"],...
  const re = new RegExp(
    "\\[\\[(\\d+)\\],\\[(\\d+)\\],\\[(\\d+)\\],\\[(\\d+)\\],\\[\"" + n + "\"\\]",
    "g"
  );
  const hits = [];
  let m;
  while ((m = re.exec(text)) && hits.length < 6) {
    hits.push({ a: +m[1], b: +m[2], c: +m[3], d: +m[4] });
  }
  console.log(n, "layoutHits=" + hits.length, JSON.stringify(hits.slice(0, 4)));
}
