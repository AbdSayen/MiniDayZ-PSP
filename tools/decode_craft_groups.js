/**
 * Decode Craft_* groups: item IDs from condition [0,[0,id]], object indices from [[0,N]] in header actions.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const objs = project[3];
const game = project[6].find((s) => s[0] === "Game_events");
const itemsJson = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "docs_gen", "items_eng.json"), "utf8")
);
const itemName = {};
itemsJson.items.forEach((i) => {
  itemName[i.id] = i.name;
});

function objLabel(idx) {
  if (idx < 0 || idx >= objs.length) return "idx?" + idx;
  const o = objs[idx];
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
  return pref[0] || "plugin" + o[1] + "_idx" + idx;
}

function isGroup(e) {
  return e && e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true && typeof e[1][1] === "string";
}

function findGroups(evts, prefix, out) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (isGroup(e)) {
      const name = e[1][1];
      if (/^Craft_/.test(name)) out.push({ name, node: e });
      for (let i = 0; i < e.length; i++) {
        if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) findGroups(e[i], name, out);
      }
    } else {
      for (let i = 0; i < e.length; i++) {
        if (Array.isArray(e[i])) findGroups(e[i], prefix, out);
      }
    }
  }
}

function collectItemRefs(node, acc) {
  const s = JSON.stringify(node);
  for (const m of s.matchAll(/\[0,\[0,(\d+)\]\]/g)) {
    const id = parseInt(m[1], 10);
    if (id >= 1 && id <= 500) acc.add(id);
  }
}

function collectObjIndices(node, acc) {
  const s = JSON.stringify(node);
  for (const m of s.matchAll(/\[20,536,119,false,null,\[\[0,(\d+)\]\]\]/g)) {
    acc.add(parseInt(m[1], 10));
  }
}

const groups = [];
findGroups(game[1], "", groups);
groups.sort((a, b) => a.name.localeCompare(b.name));

const uidMap = {};
const recipes = {};

for (const g of groups) {
  const items = new Set();
  const objIdx = new Set();
  collectItemRefs(g.node, items);
  collectObjIndices(g.node, objIdx);
  recipes[g.name] = {
    itemIds: [...items].sort((a, b) => a - b),
    itemNames: [...items].sort((a, b) => a - b).map((id) => id + ":" + (itemName[id] || "?")),
    headerObjIndices: [...objIdx].sort((a, b) => a - b),
    headerObjLabels: [...objIdx].sort((a, b) => a - b).map((i) => i + ":" + objLabel(i)),
  };
  objIdx.forEach((i) => {
    if (!uidMap[i]) uidMap[i] = objLabel(i);
  });
}

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.writeFileSync(path.join(outDir, "craft_uid_map.json"), JSON.stringify(uidMap, null, 2));
fs.writeFileSync(path.join(outDir, "craft_recipes_decoded.json"), JSON.stringify(recipes, null, 2));

console.log("Craft groups:", groups.length);
console.log("\n=== UID map (object type INDEX -> sprite prefix) ===");
Object.keys(uidMap)
  .sort((a, b) => a - b)
  .forEach((k) => console.log(k, "->", uidMap[k]));

console.log("\n=== Recipes (item id refs only) ===");
for (const [name, r] of Object.entries(recipes)) {
  console.log(name, r.itemNames.join(", "));
}
