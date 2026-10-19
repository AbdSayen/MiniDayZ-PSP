/**
 * Walk New_bullet function ACE text for weapon IV4 string compares followed by
 * SetInstanceVar IV0 with a numeric literal (bullet base damage candidates).
 */
const fs = require("fs");
const path = require("path");

const decoded = fs.readFileSync(
  path.join(__dirname, "..", "cpp", "docs_gen", "functions", "New_bullet_decoded.txt"),
  "utf8"
);
const rawJson = fs.readFileSync(
  path.join(__dirname, "..", "cpp", "docs_gen", "functions", "New_bullet.json"),
  "utf8"
);

// Also scan full data.js near "New_bullet" for SetInstanceVar patterns
const data = fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8");

const weapons = [
  "FNX", "fnx", "Glock", "Colt", "Magnum", "AKM", "AK-74", "AKs", "M4", "Mosin",
  "SKS", "SVD", "MP5", "IZH", "Remington", "bow", "Crossbow", "Sporter", "Amphibia",
  "Repeater", "L85", "UMP", "fireaxe", "pipewrench", "crowbar", "sword", "Fist",
  "bat", "shovel", "splitaxe", "knife"
];

const results = {};
for (const w of weapons) {
  results[w] = { setIv0: [], nearNums: [] };
  let from = 0;
  let n = 0;
  while (n < 20) {
    const i = data.indexOf('"' + w + '"', from);
    if (i < 0) break;
    const slice = data.slice(i, Math.min(data.length, i + 900));
    // SetInstanceVar IV0 = literal: often [10,0],...,[0,N]
    const sets = [...slice.matchAll(/\[10,0\][^\]]{0,80}\[0,(\d+(?:\.\d+)?)\]/g)];
    for (const m of sets) {
      const v = parseFloat(m[1]);
      if (v >= 5 && v <= 120) results[w].setIv0.push(v);
    }
    const nums = [...slice.matchAll(/\[0,(\d+(?:\.\d+)?)\]/g)].map((m) => parseFloat(m[1]))
      .filter((v) => v >= 8 && v <= 100);
    results[w].nearNums.push(...nums.slice(0, 8));
    from = i + w.length;
    n++;
  }
  // unique
  results[w].setIv0 = [...new Set(results[w].setIv0)].sort((a, b) => a - b);
  results[w].nearNums = [...new Set(results[w].nearNums)].sort((a, b) => a - b);
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "new_bullet_dmg.json");
fs.writeFileSync(out, JSON.stringify(results, null, 2));
console.log("wrote", out);
for (const [w, r] of Object.entries(results)) {
  if (r.setIv0.length || r.nearNums.length) {
    console.log(w, "IV0=", r.setIv0.join(",") || "-", "near=", r.nearNums.slice(0, 10).join(","));
  }
}

// Also dump lines from decoded with Fist/fireaxe and nearby digits
const lines = decoded.split("\n");
for (let i = 0; i < lines.length; i++) {
  if (/Fist|fireaxe|fnx|FNX|SetInstanceVar.*IV0/i.test(lines[i])) {
    if (i < 5 || /IV0|,10\]|,15\]|,20\]|,23\]|,25\]|,30\]|,35\]|,40\]|,45\]/.test(lines[i] + lines[i + 1])) {
      // skip noise
    }
  }
}
