/**
 * Extract all item IDs/names from localization + cross-check Plus version
 */
const fs = require("fs");
const path = require("path");

function parseItemsXml(file) {
  const xml = fs.readFileSync(file, "utf8");
  const items = [];
  const re = /<item>\s*<id>(\d+)<\/id>\s*<name>([^<]*)<\/name>\s*<\/item>/g;
  let m;
  while ((m = re.exec(xml))) {
    items.push({ id: parseInt(m[1], 10), name: m[2] });
  }
  return items;
}

function parseUiXml(file) {
  if (!fs.existsSync(file)) return [];
  const xml = fs.readFileSync(file, "utf8");
  const items = [];
  const re = /<item>\s*<id>([^<]+)<\/id>\s*<name>([^<]*)<\/name>\s*<\/item>/g;
  let m;
  while ((m = re.exec(xml))) {
    items.push({ id: m[1], name: m[2] });
  }
  return items;
}

const root = path.join(__dirname, "..");
const engItems = parseItemsXml(path.join(root, "original_docs", "l_eng_items.xml"));
const engUi = parseUiXml(path.join(root, "original_docs", "l_eng_ui.xml"));
const engLog = parseUiXml(path.join(root, "original_docs", "l_eng_log.xml"));

console.log("Items:", engItems.length);
engItems.forEach((i) => console.log(i.id + "\t" + i.name));
console.log("\nUI strings:", engUi.length);
console.log("Log strings:", engLog.length);

const outDir = path.join(root, "cpp", "docs_gen");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(
  path.join(outDir, "items_eng.json"),
  JSON.stringify({ items: engItems, ui: engUi, log: engLog }, null, 2)
);

// Also extract from Plus for comparison
const plusItemsPath = path.join(root, "docs", "l_eng_items.xml");
if (fs.existsSync(plusItemsPath)) {
  const plus = parseItemsXml(plusItemsPath);
  console.log("\nPlus items:", plus.length);
  const origIds = new Set(engItems.map((i) => i.id));
  const plusOnly = plus.filter((i) => !origIds.has(i.id));
  console.log("Plus-only items:", plusOnly.length);
  plusOnly.forEach((i) => console.log("  +" + i.id + " " + i.name));
}
