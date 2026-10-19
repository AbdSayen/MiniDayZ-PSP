/**
 * Find bullet instance IV0 SetInstanceVar near CompareInstanceVar IV4 "weaponname"
 * inside Game_events — stronger signal for per-weapon base damage.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const names = [
  "fnx", "colt", "magnum", "glock", "akm", "ak74", "aks74u", "m4", "mosin", "sks",
  "svd", "mp5", "izh", "remington", "bow", "crossbow", "amphibia", "sporter",
  "repeater", "l85", "aug", "saiga", "mac10", "ump45", "fireaxe", "sword", "fist",
  "pipewrench", "crowbar", "bat", "shovel", "sledge", "pitchfork", "pickaxe",
];

const out = {};
for (const name of names) {
  const values = new Set();
  const re = new RegExp(
    'IV4.{0,40}"' + name + '".{0,800}?SetInstanceVar.{0,40}IV0.{0,40}\\[0,(\\d+(?:\\.\\d+)?)\\]',
    "gi"
  );
  let m;
  while ((m = re.exec(text)) !== null) values.add(parseFloat(m[1]));
  // Also: IV4 compare then nearby SetInstanceVar IV0 within 1200 chars
  let from = 0;
  let n = 0;
  while (n < 15) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) break;
    from = i + name.length;
    const slice = text.slice(Math.max(0, i - 200), Math.min(text.length, i + 1200));
    if (!/IV4/.test(slice) && !/CompareInstanceVar/.test(slice)) {
      n++;
      continue;
    }
    for (const mm of slice.matchAll(/IV0.{0,60}\[0,(\d+(?:\.\d+)?)\]/g)) {
      const v = parseFloat(mm[1]);
      if (v >= 5 && v <= 120) values.add(v);
    }
    n++;
  }
  out[name] = [...values].sort((a, b) => a - b);
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "bullet_dmg_by_name.json");
fs.writeFileSync(outPath, JSON.stringify(out, null, 2));
console.log("wrote", outPath);
for (const [k, v] of Object.entries(out)) {
  if (v.length) console.log(k, v.join(", "));
}
