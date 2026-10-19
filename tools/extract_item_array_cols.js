/** Scan data.js for items master Array (obj536) column reads/writes. */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const s = fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "");

const readCols = {};
const reRead = /536,119,false,null,\[\[0,(\d+)\]\]/g;
let m;
while ((m = reRead.exec(s))) {
  readCols[m[1]] = (readCols[m[1]] || 0) + 1;
}

const read301ctx = [];
let pos = 0;
while ((pos = s.indexOf("[[0,301]]", pos + 1)) >= 0 && read301ctx.length < 5) {
  read301ctx.push(s.slice(pos - 80, pos + 60));
}

// Array SetX / SetAt targeting obj536 row from item id (119 IV0): [20,536,119,false,null,[[0,COL]]]
const writeCols = {};
const reWrite = /\[511,182,null[^\]]+\[7,\[0,(\d+)\]\]/g;
while ((m = reWrite.exec(s))) {
  writeCols[m[1]] = (writeCols[m[1]] || 0) + 1;
}

const out = {
  read_columns_by_frequency: Object.entries(readCols)
    .sort((a, b) => b[1] - a[1])
    .map(([col, n]) => ({ col: +col, n })),
  read_col_301_count: readCols["301"] || 0,
  write_setat_columns: Object.entries(writeCols)
    .sort((a, b) => b[1] - a[1])
    .map(([col, n]) => ({ col: +col, n })),
  sample_301_context: read301ctx,
};

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "item_array_cols.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
console.log("unique read cols", Object.keys(readCols).length, "301 reads", out.read_col_301_count);
console.log(
  "top read cols",
  out.read_columns_by_frequency
    .slice(0, 15)
    .map((x) => x.col + ":" + x.n)
    .join(", ")
);
