/**
 * Harvest numeric item-id literals near Spawn_drop / Check_loot / Trigger_spawn.
 */
const fs = require("fs");
const path = require("path");

const data = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");
const keys = ["Spawn_drop", "Check_loot", "Trigger_spawn", "airdrop"];
const out = {};

for (const key of keys) {
  const ids = {};
  let from = 0;
  let hits = 0;
  while (hits < 40) {
    const i = data.indexOf('"' + key + '"', from);
    if (i < 0) break;
    const slice = data.slice(i, Math.min(data.length, i + 2500));
    for (const m of slice.matchAll(/\[0,(\d+)\]/g)) {
      const n = parseInt(m[1], 10);
      if (n >= 1 && n <= 500) ids[n] = (ids[n] || 0) + 1;
    }
    from = i + key.length;
    hits++;
  }
  const ranked = Object.entries(ids)
    .map(([id, c]) => ({ id: +id, c }))
    .sort((a, b) => b.c - a.c)
    .slice(0, 40);
  out[key] = ranked;
  console.log(key, "hits", hits, "top", ranked.slice(0, 15).map((x) => x.id + "x" + x.c).join(","));
}

fs.writeFileSync(
  path.join(__dirname, "..", "cpp", "docs_gen", "spawn_drop_ids.json"),
  JSON.stringify(out, null, 2)
);
