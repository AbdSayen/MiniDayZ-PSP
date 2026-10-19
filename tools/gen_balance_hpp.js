const fs = require("fs");
const path = require("path");

const root = path.join(__dirname, "..");
const g = JSON.parse(fs.readFileSync(path.join(root, "cpp", "docs_gen", "globals.json"), "utf8"));

let hpp =
  "#pragma once\n" +
  "// Auto-extracted from original_docs/data.js — Mini DAYZ 1.4.1\n" +
  "// Do not hand-edit balance without verifying against source.\n\n" +
  "namespace mdaz {\nnamespace balance {\n\n";

g.globals.forEach((x) => {
  if (x.type === "number" && typeof x.init === "number") {
    const n = x.name.replace(/[^A-Za-z0-9_]/g, "_");
    hpp += "constexpr double " + n + " = " + x.init + ";\n";
  }
});

hpp += "\n// Text globals (initial values)\n";
g.globals
  .filter((x) => x.type === "text")
  .forEach((x) => {
    const n = x.name.replace(/[^A-Za-z0-9_]/g, "_");
    const v = String(x.init).replace(/\\/g, "\\\\").replace(/"/g, '\\"');
    hpp += "constexpr const char* " + n + ' = "' + v + '";\n';
  });

hpp += "\n} // namespace balance\n} // namespace mdaz\n";

const includeDir = path.join(root, "cpp", "include", "mdaz");
fs.mkdirSync(includeDir, { recursive: true });
fs.writeFileSync(path.join(includeDir, "balance_globals.hpp"), hpp);
console.log("Wrote balance_globals.hpp", hpp.length, "bytes");

const itemsPath = path.join(root, "cpp", "docs_gen", "items_eng.json");
if (fs.existsSync(itemsPath)) {
  const items = JSON.parse(fs.readFileSync(itemsPath, "utf8"));
  const dataDir = path.join(root, "cpp", "data");
  fs.mkdirSync(dataDir, { recursive: true });
  fs.writeFileSync(
    path.join(dataDir, "items.json"),
    JSON.stringify({ version: "1.4.1", items: items.items }, null, 2)
  );
  console.log("Wrote items.json count=", items.items.length);
}
