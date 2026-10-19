const fs = require("fs");
const path = require("path");
const raw = fs.readFileSync(path.join(__dirname, "../original_docs/data.js"), "utf8");
const project = JSON.parse(raw.replace(/^\uFEFF/, "")).project;
const sheet = project[6].find((s) => s[0] === 0);
const evts = sheet[1];

const names = [
  "gui_car_bg",
  "gui_car_fuel",
  "gui_car_condition",
  "gui_btn_car_leave",
  "gui_btn_car_forward",
  "bar_fuel",
  "bar_hp",
  "gui_car_nofuel_icon",
  "gui_car_disable_icon",
];

function walk(n, d, out) {
  if (d > 30 || !Array.isArray(n)) return;
  if (n.length >= 6 && typeof n[5] === "string" && names.includes(n[5])) {
    out.push({ name: n[5], x: n[0], y: n[1], w: n[2], h: n[3] });
  }
  for (const c of n) walk(c, d + 1, out);
}

const found = [];
for (const e of evts) walk(e, 0, found);
console.log(JSON.stringify(found, null, 2));
