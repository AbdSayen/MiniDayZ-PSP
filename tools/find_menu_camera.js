const fs = require("fs");
const path = require("path");

const project = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "")
).project;

// Layout sheets: project[5] entries like "GUI_layout,1024,768,..."
const layouts = project[5];
console.log("layout entries:", layouts.length);
layouts.forEach((L, i) => {
  if (String(L[0]).includes("Menu") || String(L[0]).includes("menu")) {
    console.log("layout", i, L[0], "size", L[1], L[2]);
  }
});

const menuLayout = layouts.find((L) => L[0] === "Menu");
if (menuLayout) {
  const types = project[3]; // type table
  const instances = menuLayout[4] || menuLayout[3];
  console.log("Menu layout fields", menuLayout.slice(0, 6));
}

// Search all types for camera_menu
const types = project[3];
types.forEach((t, i) => {
  const name = t[0];
  if (name && (String(name).includes("camera") || String(name).includes("menu_bg"))) {
    console.log("type", i, name);
  }
});

// Menu_Events: behaviors on camera — search any string with camera
const hits = [];
function walk(evts) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const s = JSON.stringify(e);
    if (/camera/i.test(s) && s.length < 12000) hits.push({ len: s.length, s });
    for (const x of e) walk(x);
  }
}
const menuEv = project[6].find((sh) => sh[0] === "Menu_Events");
if (menuEv) walk(menuEv[1]);
hits.sort((a, b) => a.len - b.len);
console.log("\nMenu_Events camera-ish blocks:", hits.length);
hits.slice(0, 5).forEach((h, i) => {
  const strs = new Set();
  (function w(x) {
    if (typeof x === "string") strs.add(x);
    else if (Array.isArray(x)) x.forEach(w);
  })(JSON.parse(h.s));
  console.log(i, "len", h.len, [...strs].filter((x) => x.length < 40).join(" | "));
});
