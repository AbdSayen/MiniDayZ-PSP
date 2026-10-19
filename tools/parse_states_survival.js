/**
 * Walk States / Update_Stats function JSON and extract subtrees mentioning survival vars.
 */
const fs = require("fs");
const path = require("path");

function loadFn(name) {
  const p = path.join(__dirname, "..", "cpp", "docs_gen", "functions", name + ".json");
  if (!fs.existsSync(p)) {
    // regenerate via extract
    require("child_process").execSync('node tools/extract_function_body.js "' + name + '"', {
      cwd: path.join(__dirname, ".."),
      stdio: "ignore",
    });
  }
  return JSON.parse(fs.readFileSync(p, "utf8"));
}

const keys = [
  "Starving_current",
  "Thirsty_current",
  "Regeneration_current",
  "Weathercold_current",
  "Player_temperature",
  "Hunger",
  "Thirst",
  "Cold",
  "Bleed",
  "Stats_blood_lost",
  "Death_log_reason",
  "Night_cold",
  "Rain_cold",
  "Day_cold",
  "Day_mode",
  "perk_camel",
  "perk_hamster",
  "perk_regen",
  "perk_snowborn",
  "perk_metabolism",
];

function collectWindows(node, pathAcc, out) {
  const s = JSON.stringify(node);
  for (const k of keys) {
    if (s.indexOf('"' + k + '"') >= 0 && s.length < 20000) {
      // extract nearby numbers and strings
      const lit = [];
      (function w(n, d) {
        if (d > 14) return;
        if (typeof n === "string" || typeof n === "number") lit.push(n);
        else if (Array.isArray(n)) n.forEach((x) => w(x, d + 1));
      })(node, 0);
      out.push({
        key: k,
        path: pathAcc,
        strings: [...new Set(lit.filter((x) => typeof x === "string"))].slice(0, 40),
        numbers: [...new Set(lit.filter((x) => typeof x === "number" && Math.abs(x) < 1e5))]
          .sort((a, b) => a - b)
          .slice(0, 50),
        raw: s.slice(0, 500),
      });
      return; // one entry per node
    }
  }
  if (Array.isArray(node)) {
    node.forEach((c, i) => {
      if (Array.isArray(c)) collectWindows(c, pathAcc + "/" + i, out);
    });
  }
}

for (const fn of ["States", "Update_Stats", "Player_get_hit"]) {
  console.log("\n========", fn, "========");
  let data;
  try {
    data = loadFn(fn);
  } catch (e) {
    console.log("skip", e.message);
    continue;
  }
  const out = [];
  collectWindows(data, "root", out);
  // dedupe by key+numbers signature
  const seen = new Set();
  out.forEach((o) => {
    const sig = o.key + ":" + o.numbers.slice(0, 15).join(",");
    if (seen.has(sig)) return;
    seen.add(sig);
    console.log(
      o.key,
      "nums[",
      o.numbers.filter((n) => n !== 0 && n !== -1 && Math.abs(n) < 500).slice(0, 20).join(", "),
      "] strs[",
      o.strings.filter((s) => keys.includes(s) || /Hunger|Thirst|Cold|Bleed|Death|blood|temp|perk/i.test(s)).join(", "),
      "]"
    );
  });
}
