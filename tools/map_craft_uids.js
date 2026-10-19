/**
 * Map Craft_* event object UIDs to C2 object types and item hints.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const objs = project[3];

function imagePrefix(o) {
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
  return [...new Set(pref)];
}

const byId = {};
objs.forEach((o, idx) => {
  byId[o[0]] = { idx, o };
});

const uids = [308, 309, 310, 311, 317, 334, 315, 389, 536, 283, 151, 925, 6];

console.log("=== By C2 type SID (o[0]) ===");
for (const uid of uids) {
  const e = byId[uid];
  if (!e) console.log(uid, "SID not found");
  else {
    const pref = imagePrefix(e.o);
    console.log(JSON.stringify({ uid, kind: "sid", idx: e.idx, plugin: e.o[1], prefixes: pref, ivNames: e.o[3] }));
  }
}

console.log("\n=== By object type INDEX (event [0,N] often = index) ===");
for (const idx of uids) {
  if (idx >= objs.length) {
    console.log(idx, "index OOR");
    continue;
  }
  const o = objs[idx];
  const pref = imagePrefix(o);
  console.log(JSON.stringify({ index: idx, sid: o[0], plugin: o[1], prefixes: pref, ivNames: o[3] }));
}

// If 536 is a family object, list related
if (byId[536]) {
  const p536 = imagePrefix(byId[536].o)[0];
  console.log("\n=== All objects with prefix like 536 ===", p536);
  objs.forEach((o) => {
    const pref = imagePrefix(o);
    if (pref.includes(p536)) console.log(o[0], pref.join(","));
  });
}

// Decode action 20,536 in c2 - search runtime for plugin 536
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);
for (const uid of [308, 309, 311, 317, 334, 389]) {
  const needle = `[20,536,119,false,null,[[0,${uid}]]]`;
  const i = text.indexOf(needle);
  console.log("\nneedle", uid, "found", i >= 0);
  if (i >= 0) {
    const slice = text.slice(Math.max(0, i - 80), i + 120);
    console.log(slice);
  }
}
