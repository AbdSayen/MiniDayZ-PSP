const fs = require("fs");
const path = require("path");
const text = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

function windows(name, n) {
  let from = 0;
  const out = [];
  while (out.length < n) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) break;
    out.push(text.slice(Math.max(0, i - 180), Math.min(text.length, i + 220)).replace(/\s+/g, " "));
    from = i + name.length;
  }
  return out;
}

["Starving_current", "Thirsty_current", "Regeneration_current", "Weathercold_current"].forEach((name) => {
  console.log("====", name);
  windows(name, 10).forEach((w, i) => {
    if (/Novice_|Regular_|Veteran_|\"41\"|SetVar/.test(w) || true) {
      console.log(i, w.slice(0, 300));
      console.log("");
    }
  });
});
