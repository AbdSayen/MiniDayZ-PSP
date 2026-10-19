/** Extract bunker_zed_spawner (obj791) Create XY from saved C2 JSON exports. */
const fs = require("fs");
const path = require("path");

const src = path.join(__dirname, "..", "cpp", "docs_gen", "functions", "Zed_check_bunker.json");
const text = fs.readFileSync(src, "utf8");
const re =
  /\[\s*4,\s*791\s*\][\s\S]*?"HotspotX"\s*\][\s\S]*?\[\s*0,\s*(\d+)\s*\][\s\S]*?"HotspotY"\s*\][\s\S]*?\[\s*0,\s*(\d+)\s*\]/g;
const pts = [];
let m;
while ((m = re.exec(text))) pts.push({ x: +m[1], y: +m[2] });
const uniq = [...new Map(pts.map((p) => [p.x + "," + p.y, p])).values()];
uniq.sort((a, b) => a.y - b.y || a.x - b.x);
const out = path.join(__dirname, "..", "cpp", "data", "bunker_zed_spawners.json");
fs.writeFileSync(
  out,
  JSON.stringify(
    {
      source: "Zed_check_bunker.json Create obj791 HotspotX/Y",
      count: uniq.length,
      spawners: uniq.map((p) => ({ x: p.x, y: p.y, tx: p.x / 8, ty: p.y / 8 })),
    },
    null,
    2
  )
);
console.log("pairs", pts.length, "unique", uniq.length, "->", out);
