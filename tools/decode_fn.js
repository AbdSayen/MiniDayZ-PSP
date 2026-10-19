/** Decode any saved function JSON with ACE names */
const fs = require("fs");
const path = require("path");
const fn = process.argv[2] || "Update_Stats";
const rt = fs.readFileSync(path.join(__dirname, "..", "original_docs", "c2runtime.js"), "utf8");
const start = rt.indexOf("cr.getObjectRefTable = function");
const end = rt.indexOf("};", start) + 2;
const body = rt.slice(start, end);
const parts = body
  .slice(body.indexOf("return [") + 8, body.lastIndexOf("]"))
  .split(",")
  .map((s) => s.trim())
  .filter(Boolean);

function ace(id) {
  const p = parts[id] || "?" + id;
  return p
    .replace("cr.system_object.prototype.cnds.", "sys.cnd.")
    .replace("cr.system_object.prototype.acts.", "sys.act.")
    .replace("cr.system_object.prototype.exps.", "sys.exp.")
    .replace("cr.plugins_.Sprite.prototype.cnds.", "spr.cnd.")
    .replace("cr.plugins_.Sprite.prototype.acts.", "spr.act.")
    .replace("cr.plugins_.Function.prototype.acts.", "fn.act.")
    .replace("cr.plugins_.Function.prototype.cnds.", "fn.cnd.");
}

function decExpr(e) {
  if (!Array.isArray(e)) return e;
  if (e[0] === 0) return e[1];
  if (e[0] === 2) return JSON.stringify(e[1]);
  if (e[0] === 23) return "G:" + e[1];
  if (e[0] === 11) return "V:" + e[1];
  if (e[0] === 7) return decExpr(e[1]);
  if (e[0] === 1) return e[1]; // float literal sometimes
  if (e[0] === 12) return "(" + decExpr(e[1]) + "==" + decExpr(e[2]) + ")";
  if (e[0] === 18) return "(" + decExpr(e[1]) + "?" + decExpr(e[2]) + ":" + decExpr(e[3]) + ")";
  if (e[0] === 4) return "rand";
  if (e[0] === 5) return "choose";
  if (e[0] === 8) return "op" + e[1];
  if (e[0] === 10) return "IV" + e[1];
  return "E" + JSON.stringify(e).slice(0, 80);
}

function decParams(params) {
  if (!Array.isArray(params)) return [];
  return params.map((p) => {
    if (!Array.isArray(p)) return p;
    if (p[0] === 10) return "IV" + p[1]; // instance var index form in acts
    if (p.length === 2 && (p[0] === 0 || p[0] === 1 || p[0] === 7)) return decExpr(p[1] !== undefined ? p : p);
    // act param: [10, ivIndex] or [7, expr]
    if (p[0] === 7) return decExpr(p[1]);
    if (typeof p[0] === "number" && p.length <= 3) return p.map(decExpr);
    return p.map((x) => (Array.isArray(x) ? decExpr(x) : x));
  });
}

const lines = [];
function walk(evts, d) {
  if (!Array.isArray(evts) || d > 14) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    const conds = e[5];
    const acts = e[6];
    if (Array.isArray(conds)) {
      conds.forEach((c) => {
        if (!Array.isArray(c)) return;
        const who = c[0] === -1 ? "sys" : "obj" + c[0];
        const name = ace(c[1]);
        const pars = decParams(c[9] || []);
        lines.push("  ".repeat(d) + "IF " + who + " " + name + " " + JSON.stringify(pars).slice(0, 140));
      });
    }
    if (Array.isArray(acts)) {
      acts.forEach((a) => {
        if (!Array.isArray(a)) return;
        const who = a[0] === -1 ? "sys" : "obj" + a[0];
        const name = ace(a[1]);
        // params often last array
        let pars = [];
        for (let i = a.length - 1; i >= 3; i--) {
          if (Array.isArray(a[i])) {
            pars = decParams(a[i]);
            break;
          }
        }
        // also common: acts have params at index with [10, iv], [7, expr]
        if (a[0] !== -1) {
          const iv = a.find((x) => Array.isArray(x) && x[0] === 10);
          const ex = a.filter((x) => Array.isArray(x) && x[0] === 7);
          if (iv || ex.length) {
            pars = [];
            if (iv) pars.push("IV" + iv[1]);
            ex.forEach((x) => pars.push(decExpr(x[1])));
          }
        }
        if (a[0] === -1 && Array.isArray(a[3])) pars = decParams(a[3]);
        lines.push("  ".repeat(d) + "DO " + who + " " + name + " " + JSON.stringify(pars).slice(0, 160));
      });
    }
    for (let i = 0; i < e.length; i++) {
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) walk(e[i], d + 1);
    }
  }
}

const data = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "docs_gen", "functions", fn + ".json"), "utf8")
);
walk([data], 0);
const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "functions", fn + "_decoded.txt");
fs.writeFileSync(outPath, lines.join("\n"));
console.log(lines.slice(0, 120).join("\n"));
console.log("... lines", lines.length, "->", outPath);
