/**
 * Find melee weapon IV4 string compares near CallFunction("NPC_get_damage", ...)
 * and harvest literal damage param candidates.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const t = fs.readFileSync(dataPath, "utf8");

const melee = [
  "Fist", "fireaxe", "pipewrench", "crowbar", "pitchfork", "pickaxe",
  "sledgehammer", "shovel", "bat", "splitaxe", "sword", "knife", "knife_army",
  "knife_hunter", "knife_butcher"
];

// ACE CallFunction packs: [2,"NPC_get_damage"],[13,[ ... params ...]]
// Damage is often param index 2 as [0, N]
const callRe = /\[2,"NPC_get_damage"\]/g;
const calls = [];
let m;
while ((m = callRe.exec(t)) !== null) {
  const start = Math.max(0, m.index - 800);
  const end = Math.min(t.length, m.index + 400);
  const window = t.slice(start, end);
  const weapons = [];
  for (const name of melee) {
    if (window.indexOf('"' + name + '"') >= 0) weapons.push(name);
  }
  const after = t.slice(m.index, Math.min(t.length, m.index + 350));
  const nums = [];
  const numRe = /\[0,(\d+(?:\.\d+)?)\]/g;
  let nm;
  while ((nm = numRe.exec(after)) !== null) {
    const v = parseFloat(nm[1]);
    if (v >= 5 && v <= 120) nums.push(v);
  }
  // Also pick-distance style compares near weapon (often [0,45] for Fist)
  const before = t.slice(start, m.index);
  const beforeNums = [];
  const bnRe = /\[0,(\d+(?:\.\d+)?)\]/g;
  while ((nm = bnRe.exec(before)) !== null) {
    const v = parseFloat(nm[1]);
    if (v >= 5 && v <= 120) beforeNums.push(v);
  }
  calls.push({ weapons, nums: nums.slice(0, 12), beforeNums: beforeNums.slice(-12) });
}

const byWeapon = {};
for (const name of melee) byWeapon[name] = { callHits: 0, dmgCandidates: {}, rangeCandidates: {} };
for (const c of calls) {
  for (const w of c.weapons) {
    byWeapon[w].callHits += 1;
    for (const n of c.nums) {
      byWeapon[w].dmgCandidates[n] = (byWeapon[w].dmgCandidates[n] || 0) + 1;
    }
  }
}

// Secondary: for each weapon string, look ~600 chars ahead for [0,N] literals 8..80
for (const name of melee) {
  let from = 0;
  while (true) {
    const i = t.indexOf('"' + name + '"', from);
    if (i < 0) break;
    const slice = t.slice(i, Math.min(t.length, i + 600));
    const re = /\[0,(\d+(?:\.\d+)?)\]/g;
    let nm;
    while ((nm = re.exec(slice)) !== null) {
      const v = parseFloat(nm[1]);
      if (v >= 8 && v <= 80) {
        byWeapon[name].rangeCandidates[v] = (byWeapon[name].rangeCandidates[v] || 0) + 1;
      }
    }
    from = i + name.length;
    if (Object.keys(byWeapon[name].rangeCandidates).length > 20) break;
  }
}

const out = path.join(__dirname, "..", "cpp", "docs_gen", "melee_npc_dmg.json");
fs.writeFileSync(out, JSON.stringify({ callCount: calls.length, byWeapon }, null, 2));
console.log("calls=", calls.length, "wrote", out);
for (const name of melee) {
  const b = byWeapon[name];
  const dmg = Object.keys(b.dmgCandidates).map(Number).sort((a, c) => a - c);
  const rng = Object.keys(b.rangeCandidates).map(Number).sort((a, c) => a - c);
  if (b.callHits || dmg.length || rng.length) {
    console.log(name, "calls=" + b.callHits, "dmg=", dmg.join(","), "near=", rng.join(","));
  }
}
