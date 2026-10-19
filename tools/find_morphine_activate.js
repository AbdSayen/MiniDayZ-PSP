const fs = require("fs");
const t = fs.readFileSync("original_docs/data.js", "utf8").replace(/^\uFEFF/, "");

// C2 item_activate param is inventory item id (see items.json alignment for 19,28,46)
for (const id of [45, 36]) {
  let idx = 0;
  let n = 0;
  while (n < 8 && (idx = t.indexOf(`CompareParam`, idx + 1)) >= 0) {
    const slice = t.slice(idx, idx + 80);
    if (slice.includes(`,${id},`) || slice.includes(`,${id}]`)) {
      const ctx = t.slice(Math.max(0, idx - 120), idx + 200);
      if (ctx.includes("180") || ctx.includes("item_activate")) {
        console.log("\n=== CompareParam?", id, "at", idx);
        console.log(ctx.replace(/\s+/g, " ").slice(0, 280));
        n++;
      }
    }
  }
}

// Spritesheet name
const mi = t.indexOf("morphine-sheet");
console.log("\nmorphine-sheet at", mi);
if (mi >= 0) {
  const ctx = t.slice(Math.max(0, mi - 800), mi + 400);
  const ids = [...ctx.matchAll(/id_(\d+)/g)].map((m) => m[1]);
  console.log("nearby id_ refs", [...new Set(ids)].slice(0, 20));
}
