/**
 * Summarize camera_roll_menu from Menu_Events (data.js).
 */
const fs = require("fs");
const path = require("path");

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

const menuEv = project[6].find((sh) => sh[0] === "Menu_Events");
let body = null;
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (s.includes("camera_roll_menu") && s.includes("Stopper") && (!body || s.length > body.length))
      body = e;
    for (const x of e) walk(x);
  }
}
walk(menuEv[1]);

const flat = JSON.stringify(body);
const nums = [...new Set(flat.match(/\[0,(\d+)\]/g)?.map((m) => +m.slice(3, -1)) || [])]
  .filter((n) => n > 0 && n < 500)
  .sort((a, b) => a - b);
console.log("camera_roll_menu block len", flat.length);
console.log("key nums", nums.join(", "));

const out = path.join(__dirname, "..", "cpp", "docs_gen", "menu_camera_decode.txt");
const lines = [
  "# camera_roll_menu (Menu_Events / Camera group)",
  "",
  "Source: original_docs/data.js → Menu_Events",
  "",
  "## Verified constants (from camera_roll_menu_impl.json)",
  "- Pin type: Buildings (layer name string in CreateObject)",
  "- Distance compare: distance(cam, pin) > 100 → reposition pin + recurse camera_roll_menu",
  "- Stopper behavior speed: 3 (layout units per tick family)",
  "- Bullet move speed toward pin: 8 and 24 in alternate branches",
  "",
  "## Pin placement expression",
  "- X: user_device_X_mid + (user_device_X - layout ref 394) via expr opcode 19,95",
  "- Y: user_device_Y_mid + (user_device_Y - layout ref 152) via expr opcode 19,95",
  "- Desktop: user_device_*_mid ≈ layout center (1250, 750) when no touch; pan driven by moving pin",
  "",
  "## Numeric literals in block",
  nums.join(", "),
];
fs.writeFileSync(out, lines.join("\n"));
console.log("wrote", out);
