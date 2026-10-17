/** Find small event blocks referencing obj172 IV index. */
const fs = require("fs");
const path = require("path");

const iv = parseInt(process.argv[2] || "19", 10);
const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;

let n = 0;
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    const needle = "[10," + iv + "]";
    if (s.includes('"Items_update"') && s.includes(needle) && s.length < 12000) {
      ++n;
      if (n <= 5) {
        fs.writeFileSync(
          path.join(__dirname, "..", "cpp", "docs_gen", "iv" + iv + "_items_update_" + n + ".json"),
          JSON.stringify(e, null, 2)
        );
      }
    }
    for (const x of e) walk(x);
  }
}
project[6].forEach((sh) => walk(sh[1]));
console.log("hits with Items_update and IV" + iv, n);
