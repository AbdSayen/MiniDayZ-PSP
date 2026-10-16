/**
 * Find numeric literals associated with named globals / function strings in event sheets.
 * Looks for windows around known variable names.
 */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
const game = project[6].find((s) => s[0] === "Game_events");
const text = JSON.stringify(game);

const targets = [
  "Starving_current",
  "Thirsty_current",
  "Regeneration_current",
  "Weathercold_current",
  "Bleed",
  "Player_temperature",
  "Weather_temperature",
  "Novice_starving",
  "Regular_starving",
  "Veteran_starving",
  "Stats_time_lived",
  "Timer_ALL",
  "Timer_Hours",
  "Timer_Minutes",
  "Day_Night_speed",
  "Rain_speed",
  "Day_mode",
  "rains_level",
  "Zed_dmg",
  "Wolf_dmg",
  "Zed_army_dmg",
  "Zed_fast_dmg",
  "dmg_randomizer",
  "Player_get_hit",
  "NPC_get_damage",
  "Bullets_dmg_deal",
  "Range_less_power",
  "Range_destroy",
  "New_bullet",
  "dispersion_pershot",
  "Check_loot",
  "Spawn_drop",
  "Trigger_spawn",
  "LOOT_RESPAWN_TIME",
  "Airdrop_recharge",
  "create_map_location",
  "generate_location",
  "generate_helicrashes",
  "Map_size_X",
  "Map_size_Y",
  "Update_Stats",
  "States",
];

const results = {};

for (const name of targets) {
  const windows = [];
  let from = 0;
  while (windows.length < 12) {
    const i = text.indexOf('"' + name + '"', from);
    if (i < 0) break;
    const slice = text.slice(Math.max(0, i - 200), Math.min(text.length, i + 250));
    const nums = [...slice.matchAll(/(?<![A-Za-z_])(-?\d+(?:\.\d+)?)(?![A-Za-z_])/g)].map((m) =>
      parseFloat(m[1])
    );
    // Keep interesting magnitudes
    const interesting = nums.filter(
      (n) =>
        (Math.abs(n) > 0 && Math.abs(n) < 10000 && n !== -1) ||
        n === 0.72 ||
        (n > 0 && n < 1)
    );
    windows.push({
      nums: interesting.slice(0, 30),
      preview: slice.replace(/\s+/g, " ").slice(0, 220),
    });
    from = i + name.length;
  }
  results[name] = { hits: windows.length, windows };
}

const outDir = path.join(__dirname, "..", "cpp", "docs_gen");
fs.writeFileSync(path.join(outDir, "var_assignments.json"), JSON.stringify(results, null, 2));

for (const name of targets) {
  const r = results[name];
  if (!r.hits) {
    console.log(name, "NO HITS");
    continue;
  }
  const allNums = [];
  r.windows.forEach((w) => allNums.push(...w.nums));
  const uniq = [...new Set(allNums)].sort((a, b) => a - b);
  console.log(name, "hits=" + r.hits, "nums=", uniq.slice(0, 25).join(","));
}
