/** Parse generate_location.json (text) for obj518 SetTile (action 182) offsets + tile ids. */
const fs = require("fs");
const path = require("path");

const src = path.join(__dirname, "..", "cpp", "docs_gen", "functions", "generate_location.json");
const text = fs.readFileSync(src, "utf8");

// Block: 518, 182, ... hotspotX/Y deltas then tile id in [0,[0,N]]
const re =
  /518,\s*182,[\s\S]*?"hotspotX"[\s\S]*?\[\s*0,\s*(-?\d+)\s*\][\s\S]*?"hotspotY"[\s\S]*?\[\s*0,\s*(-?\d+)\s*\][\s\S]*?\[\s*0,\s*\[\s*0,\s*(\d+)\s*\]\s*\]/g;

const stamps = [];
let m;
while ((m = re.exec(text))) {
  stamps.push({ dx: +m[1], dy: +m[2], tile: +m[3] });
}

const out = path.join(__dirname, "..", "cpp", "data", "generate_location_forest_stamps.json");
fs.writeFileSync(
  out,
  JSON.stringify(
    {
      source: "generate_location.json obj518 SetTile (approx; export JSON truncated)",
      count: stamps.length,
      stamps,
    },
    null,
    2
  )
);
console.log("stamps", stamps.length, "->", out);
if (stamps.length) console.log("sample", stamps.slice(0, 8));
