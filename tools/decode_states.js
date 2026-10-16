/**
 * Decode States function using cr.getObjectRefTable ACE indices from c2runtime.js
 */
const fs = require("fs");
const path = require("path");
const vm = require("vm");

// Minimal stub to extract getObjectRefTable
const rt = fs.readFileSync(path.join(__dirname, "..", "original_docs", "c2runtime.js"), "utf8");
const start = rt.indexOf("cr.getObjectRefTable = function");
const end = rt.indexOf("};", start) + 2;
const tableSrc = rt.slice(start, end);

const sandbox = {
  cr: {
    plugins_: new Proxy(
      {},
      {
        get: (_, name) =>
          new Proxy(
            {},
            {
              get: (__, name2) =>
                new Proxy(
                  {},
                  {
                    get: (___, name3) => ({ _ref: "plugins_." + name + "." + name2 + "." + name3 }),
                  }
                ),
            }
          ),
      }
    ),
    behaviors: new Proxy(
      {},
      {
        get: (_, name) =>
          new Proxy(
            {},
            {
              get: (__, name2) =>
                new Proxy(
                  {},
                  {
                    get: (___, name3) => ({ _ref: "behaviors." + name + "." + name2 + "." + name3 }),
                  }
                ),
            }
          ),
      }
    ),
    system_object: {
      prototype: {
        cnds: new Proxy({}, { get: (_, n) => ({ _ref: "system.cnds." + n }) }),
        acts: new Proxy({}, { get: (_, n) => ({ _ref: "system.acts." + n }) }),
        exps: new Proxy({}, { get: (_, n) => ({ _ref: "system.exps." + n }) }),
      },
    },
  },
};

vm.runInNewContext(tableSrc + "\nthis.table = cr.getObjectRefTable();", sandbox);
const table = sandbox.table;

function aceName(id) {
  const e = table[id];
  if (!e) return "?" + id;
  if (e._ref) return e._ref;
  return String(e);
}

function decodeExpr(e) {
  if (e == null) return null;
  if (typeof e !== "object") return e;
  if (!Array.isArray(e)) return e;
  // expression node types in C2: [type, ...]
  // type 0 = number literal [0, value]
  // type 2 = string [2, "text"]
  // type 11? global? common: [11, "varname"] in conditions
  // type 23 = global var value [23, "name"]
  // type 7 = nested expression wrapper sometimes
  if (e[0] === 0) return e[1];
  if (e[0] === 2) return JSON.stringify(e[1]);
  if (e[0] === 23) return "global:" + e[1];
  if (e[0] === 11) return "var:" + e[1];
  if (e[0] === 7) return decodeExpr(e[1]);
  if (e[0] === 4) return "random";
  if (e[0] === 5) return "choose";
  // binary ops etc — stringify compact
  return "expr(" + JSON.stringify(e).slice(0, 120) + ")";
}

function decodeParams(params) {
  if (!Array.isArray(params)) return [];
  return params.map((p) => {
    if (!Array.isArray(p)) return p;
    // param often [mode, expression]
    if (p.length === 2) return decodeExpr(p[1] !== undefined ? p[1] : p);
    if (p[0] === 11 || p[0] === 23 || p[0] === 0 || p[0] === 2) return decodeExpr(p);
    // CompareVar style [[11,"name"],[8,op],[7,expr]]
    return p.map((x) => (Array.isArray(x) ? decodeExpr(x) : x));
  });
}

function walkEvents(evts, depth, lines) {
  if (!Array.isArray(evts) || depth > 12) return;
  for (const e of evts) {
    if (!Array.isArray(e)) continue;
    if (e[0] === 0 && Array.isArray(e[1]) && e[1][0] === true) {
      lines.push("  ".repeat(depth) + "GROUP " + e[1][1]);
    }
    // conditions at index 5 typically for ordinary events
    const conds = e[5];
    const acts = e[6];
    const subs = e[7];
    if (Array.isArray(conds)) {
      conds.forEach((c) => {
        if (!Array.isArray(c)) return;
        if (c[0] === -1) {
          lines.push(
            "  ".repeat(depth) +
              "IF system." +
              aceName(c[1]).replace(/^system\.cnds\./, "") +
              " " +
              JSON.stringify(decodeParams(c[9] || c[c.length - 1])).slice(0, 160)
          );
        } else if (typeof c[0] === "number") {
          lines.push(
            "  ".repeat(depth) +
              "IF obj" +
              c[0] +
              " " +
              aceName(c[1]) +
              " " +
              JSON.stringify(decodeParams(c[9] || [])).slice(0, 120)
          );
        }
      });
    }
    if (Array.isArray(acts)) {
      acts.forEach((a) => {
        if (!Array.isArray(a)) return;
        if (a[0] === -1) {
          lines.push(
            "  ".repeat(depth) +
              "DO system." +
              aceName(a[1]).replace(/^system\.acts\./, "") +
              " " +
              JSON.stringify(decodeParams(a[3] || a[a.length - 1])).slice(0, 160)
          );
        } else if (typeof a[0] === "number") {
          const params = a.filter((x, i) => i >= 3 && Array.isArray(x));
          lines.push(
            "  ".repeat(depth) +
              "DO obj" +
              a[0] +
              " " +
              aceName(a[1]) +
              " " +
              JSON.stringify(decodeParams(a[a.length - 1] || [])).slice(0, 160)
          );
        }
      });
    }
    if (Array.isArray(subs)) walkEvents(subs, depth + 1, lines);
    // also search nested arrays
    for (let i = 0; i < e.length; i++) {
      if (i === 5 || i === 6 || i === 7) continue;
      if (Array.isArray(e[i]) && e[i].length && Array.isArray(e[i][0])) {
        walkEvents(e[i], depth + 1, lines);
      }
    }
  }
}

const states = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "cpp", "docs_gen", "functions", "States.json"), "utf8")
);
const lines = [];
walkEvents([states], 0, lines);
const out = lines.join("\n");
fs.writeFileSync(path.join(__dirname, "..", "cpp", "docs_gen", "functions", "States_decoded.txt"), out);
console.log(out.slice(0, 8000));
console.log("\n... total lines", lines.length);
