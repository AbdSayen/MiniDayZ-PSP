/**
 * Document alternate C2 event paths for firearm bullet IV0 (no promotion unless unambiguous).
 * Outputs: cpp/docs_gen/weapon_iv0_path_scan.json + weapon_iv0_path_scan.txt
 */
const fs = require("fs");
const path = require("path");

const root = path.join(__dirname, "..");
const t = fs.readFileSync(path.join(root, "original_docs", "data.js"), "utf8").replace(/^\uFEFF/, "");

const tagToId = {
  fnx: 151, colt: 152, magnum: 153, izh: 154, remington: 155, mosin: 156, sks: 157,
  m4: 158, akm: 159, ak74: 160, aks74u: 161, l85: 162, bow: 163, crossbow: 164,
  amphibia: 165, sporter: 166, repeater: 167, svd: 168, mp5: 169, glock: 170,
  pm: 181, pb: 182, bizon: 183, groza: 184, vss: 185, sv98: 186, madsen: 187,
  saiga: 178, mac10: 179, ump45: 173, aug: 177, rpk: 174, fncal: 175,
};

function iv0FromSlice(slice) {
  const out = new Set();
  for (const mm of slice.matchAll(/\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g)) {
    const v = parseFloat(mm[1]);
    if (v >= 5 && v <= 120) out.add(v);
  }
  return out;
}

const report = {
  generated: new Date().toISOString(),
  paths: {},
  sverka_candidates: {},
  notes: [],
};

// Path A: Game_events bullet 910 action 88 SetInstanceVar IV0 (FNX proof)
{
  const re = /910,88,null[^\]]{0,160}\[10,0\],\[7,\[0,(\d+(?:\.\d+)?)\]\]/g;
  const byVal = {};
  let m;
  while ((m = re.exec(t))) {
    const v = m[1];
    byVal[v] = (byVal[v] || 0) + 1;
  }
  report.paths.game_events_910_88_iv0 = byVal;
}

// Path A2: any [10,0] IV0 literal within 4k of IV1←172.151 (FNX sverka chain)
{
  const needle = "[20,172,151,false,null]";
  const byWeapon = {};
  let idx = 0;
  while ((idx = t.indexOf(needle, idx)) >= 0) {
    const slice = t.slice(Math.max(0, idx - 500), idx + 4000);
    const iv0s = iv0FromSlice(slice);
    if (iv0s.size) byWeapon["151"] = [...iv0s].sort((a, b) => a - b);
    idx++;
  }
  report.paths.iv1_172_151_window_iv0 = byWeapon;
  const fnx = byWeapon["151"];
  if (fnx && fnx.length === 1 && fnx[0] === 23) {
    report.sverka_candidates["151"] = {
      iv0: 23,
      path: "New_bullet/Game_events: IV1←172.151 window → sole IV0 literal 23 on bullet 910",
    };
  }
}

// Path B: player_weapons (909) fire window → nearby 910 IV0 literals per weapon tag
{
  const byWeapon = {};
  for (const [tag, itemId] of Object.entries(tagToId)) {
    const values = new Set();
    let from = 0;
    for (let n = 0; n < 60; n++) {
      const i = t.indexOf('"' + tag + '"', from);
      if (i < 0) break;
      from = i + tag.length + 1;
      const slice = t.slice(Math.max(0, i - 600), Math.min(t.length, i + 3000));
      if (!slice.includes("909") && !slice.includes("910") && !slice.includes("New_bullet")) continue;
      iv0FromSlice(slice).forEach((v) => values.add(v));
    }
    byWeapon[itemId] = { tag, iv0_candidates: [...values].sort((a, b) => a - b) };
    if (values.size === 1) {
      report.rejected_ambiguous = report.rejected_ambiguous || {};
      report.rejected_ambiguous[String(itemId)] = {
        iv0: [...values][0],
        tag,
        reason: "single IV0 in tag window without IV1←172 pairing — not promoted",
      };
    }
  }
  report.paths.player_weapons_tag_windows = byWeapon;
}

// Path C: New_bullet — CompareInstanceVar IV1 to 172.itemId branches with Set IV0 on 910
{
  const weaponIds = new Set(Object.values(tagToId));
  const hits = {};
  for (const wid of weaponIds) {
    const needle = `[20,172,${wid},false,null]`;
    let idx = 0;
    const iv0s = new Set();
    while ((idx = t.indexOf(needle, idx)) >= 0) {
      const slice = t.slice(idx, Math.min(t.length, idx + 4000));
      if (!slice.includes("New_bullet") && !slice.includes("910")) {
        idx++;
        continue;
      }
      iv0FromSlice(slice).forEach((v) => iv0s.add(v));
      idx++;
    }
    if (iv0s.size) hits[wid] = [...iv0s].sort((a, b) => a - b);
  }
  report.paths.new_bullet_iv1_weapon_branches = hits;
}

// Path D: player_weapons CompareInstanceVar IV0 (909) — numeric compares only
{
  const re = /909[^\]]{0,80}CompareInstanceVar[^\]]*IV0[^\]]*\[8,0\],(\d+)/g;
  const nums = {};
  let m;
  while ((m = re.exec(t))) {
    nums[m[1]] = (nums[m[1]] || 0) + 1;
  }
  report.paths.player_weapons_909_iv0_compare = nums;
  report.notes.push(
    "909 IV0 CompareInstanceVar counts are weapon-type tags (strings in export), not damage literals."
  );
}

// Path E: Bullets_parameters group — any IV0 set on 910 in group subtree
{
  const gi = t.indexOf('"Bullets parameters"');
  const slice = gi >= 0 ? t.slice(gi, gi + 120000) : "";
  const iv0s = iv0FromSlice(slice);
  report.paths.bullets_parameters_group_iv0_literals = [...iv0s].sort((a, b) => a - b);
  report.notes.push("Bullets_parameters slice: IV0 literals are travel/decay, not per-weapon base damage.");
}

const outJson = path.join(root, "cpp", "docs_gen", "weapon_iv0_path_scan.json");
fs.writeFileSync(outJson, JSON.stringify(report, null, 2));

const lines = [];
lines.push("# Weapon IV0 path scan (auto)");
lines.push("Generated: " + report.generated);
lines.push("");
lines.push("## Sverka promoted (unambiguous only)");
for (const [id, s] of Object.entries(report.sverka_candidates).sort((a, b) => +a[0] - +b[0])) {
  lines.push(`- **${id}**: IV0=${s.iv0} — ${s.path}`);
}
const legacy = path.join(root, "cpp", "docs_gen", "weapon_bullet_iv0_sverka.json");
if (fs.existsSync(legacy)) {
  const prev = JSON.parse(fs.readFileSync(legacy, "utf8"));
  for (const [id, s] of Object.entries(prev)) {
    if (s.sverka) lines.push(`- **${id}**: IV0=${s.iv0} — ${s.path} (prior decode)`);
  }
}
lines.push("");
lines.push("## Rejected (ambiguous tag-window IV0)");
for (const [id, s] of Object.entries(report.rejected_ambiguous || {}).sort((a, b) => +a[0] - +b[0])) {
  lines.push(`- ${id} (${s.tag}): IV0=${s.iv0} — ${s.reason}`);
}
const fnx = report.paths.player_weapons_tag_windows?.["151"];
if (fnx && fnx.iv0_candidates?.length > 1) {
  lines.push(`- 151 (fnx): candidates ${fnx.iv0_candidates.join(", ")} — need IV1←172.151 + sole literal (see New_bullet_decoded)`);
}
lines.push("");
lines.push("## Path summaries");
lines.push("- game_events_910_88: " + JSON.stringify(report.paths.game_events_910_88_iv0));
lines.push("- bullets_parameters IV0 literals (5–120): " + report.paths.bullets_parameters_group_iv0_literals.join(", "));
lines.push("- new_bullet branches with IV0: " + Object.keys(report.paths.new_bullet_iv1_weapon_branches || {}).length + " weapon ids");
lines.push("");
for (const n of report.notes) lines.push("- " + n);

const outTxt = path.join(root, "cpp", "docs_gen", "weapon_iv0_path_scan.txt");
fs.writeFileSync(outTxt, lines.join("\n"));
console.log("wrote", outJson);
console.log("wrote", outTxt);
console.log("sverka count", Object.keys(report.sverka_candidates).length);
