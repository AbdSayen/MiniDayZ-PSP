// Port accuracy audit: compares C2 export identifiers with the C++ sources.
// Usage: node tools/audit_port.js   (run from repo root)
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..');
const data = fs.readFileSync(path.join(root, 'original_docs/data.js'), 'utf8');

function readTree(dir) {
  const out = [];
  (function walk(p) {
    for (const f of fs.readdirSync(p)) {
      const q = path.join(p, f);
      if (fs.statSync(q).isDirectory()) walk(q);
      else if (/\.(cpp|hpp)$/.test(f)) out.push(fs.readFileSync(q, 'utf8'));
    }
  })(dir);
  return out.join('\n');
}
const cpp = readTree(path.join(root, 'cpp/src')) + readTree(path.join(root, 'cpp/include'));

function idsMatching(re) {
  const s = new Set();
  let m;
  while ((m = re.exec(data))) s.add(m[1]);
  return [...s].sort();
}
function report(title, ids, filter) {
  const list = filter ? ids.filter(filter) : ids;
  const hit = list.filter((n) => cpp.includes(n));
  const pct = list.length ? Math.round((100 * hit.length) / list.length) : 0;
  console.log(`${title}: ${hit.length}/${list.length} referenced in C++ (${pct}%)`);
  return { list, hit, miss: list.filter((n) => !cpp.includes(n)) };
}

const ev = require(path.join(root, 'cpp/docs_gen/event_groups.json'));
const fnNames = ev.functionNames.map((x) => (typeof x === 'string' ? x : x.name));
const fns = report('C2 functions', fnNames);

const gNames = [...new Set(ev.groups.map((g) => (typeof g === 'string' ? g : g.name)))]
  .filter((n) => n && n.length > 3 && /[A-Z_]/.test(n));
const noise = /^(DeltaDNA|Ach_|ach_|char_unlock|user_|Loader_|Menu_|menu_|MP_|token_|login|Local_|tutor|Tutor|sav|SEED_|TD_|testbanner|Language|System_language|authorised|MyUsername|sessionID|good_login|Dragging|logging|switching|suspended|curka|gui_|GUI_|Pad_|Platform|Text_color|androidexit|Debug_mode|God_mode|Ally_video)/;
const gl = report('C2 gameplay globals (UI/online/achievements excluded)', gNames, (n) => !noise.test(n));

const craft = report('Craft_* ids in data.js', idsMatching(/"(Craft_[A-Za-z0-9_]+)"/g));
const perks = report('perk_* globals', idsMatching(/"(perk_[a-z]+)"/g), (n) => !/header|cost/.test(n));

console.log('\nMissing perks:', perks.miss.join(', '));
console.log('Missing Craft ids:', craft.miss.join(', '));
console.log('\nUnported C2 functions (' + fns.miss.length + '):\n  ' + fns.miss.join(', '));
console.log('\nUnported gameplay globals (' + gl.miss.length + '):\n  ' + gl.miss.join(', '));
