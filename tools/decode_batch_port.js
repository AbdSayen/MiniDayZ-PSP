/**
 * Batch decode: create_map_element/obj501, mag capacities, vehicle fuel, bullet hit zones.
 * Usage: node tools/decode_batch_port.js
 */
const fs = require("fs");
const path = require("path");

const root = path.join(__dirname, "..");
const dataPath = path.join(root, "original_docs", "data.js");
const outDir = path.join(root, "cpp", "docs_gen");
const project = JSON.parse(fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "")).project;
fs.mkdirSync(outDir, { recursive: true });

function walk(evts, fn) {
  if (!Array.isArray(evts)) return;
  for (const e of evts) {
    if (Array.isArray(e)) fn(e);
    if (Array.isArray(e)) for (const x of e) if (Array.isArray(x)) walk(x, fn);
  }
}

// --- 1) create_map_element: tile id -> obj501 IV10 assignments ---
function decodeCreateMapElement() {
  const branches = [];
  walk(project[6], (e) => {
    if (e[0] !== 0) return;
    const js = JSON.stringify(e);
    if (!js.includes('"create_map_element"')) return;
    if (!js.includes("501") || !js.includes("518")) return;
    // Extract pairs [10, N] and tile compare [0, tile]
    const iv10 = [];
    const tiles = [];
    (function scan(n) {
      if (!Array.isArray(n)) return;
      if (n.length >= 2 && n[0] === 10 && typeof n[1] === "number") iv10.push(n[1]);
      if (n.length >= 2 && n[0] === 0 && typeof n[1] === "number" && n[1] >= 0 && n[1] <= 20) {
        if (JSON.stringify(n).includes("150")) tiles.push(n[1]);
      }
      for (const x of n) if (Array.isArray(x)) scan(x);
    })(e);
    if (iv10.length && tiles.length) branches.push({ iv10: [...new Set(iv10)], tiles: [...new Set(tiles)], len: js.length });
  });
  branches.sort((a, b) => a.len - b.len);
  const lines = ["# create_map_element tile → obj501 IV10 (from data.js event trees)", ""];
  const map = new Map();
  for (const b of branches) {
    for (let i = 0; i < Math.min(b.iv10.length, b.tiles.length); ++i) {
      const t = b.tiles[i];
      const iv = b.iv10[i];
      if (!map.has(t)) map.set(t, new Set());
      map.get(t).add(iv);
    }
  }
  for (const [t, set] of [...map.entries()].sort((a, b) => a[0] - b[0])) {
    lines.push(`tile_${t} → IV10 ${[...set].sort((a, b) => a - b).join("|")}`);
  }
  lines.push("", `event_trees_scanned: ${branches.length}`);
  if (branches[0]) lines.push("", "## smallest tree sample", JSON.stringify(branches[0], null, 2));

  // map_element layout instances (type 501)
  const inst501 = [];
  const layouts = project[1] ? project : project;
  // C2: project[1] is name; layouts in project[7] or project[8] — scan all arrays for uid 501
  function scanLayout(arr, depth) {
    if (!Array.isArray(arr) || depth > 12) return;
    const js = JSON.stringify(arr);
    if (js.includes('"t501"') || (arr[0] === 501 && typeof arr[1] === "string")) {
      if (js.length < 8000) inst501.push(arr);
    }
    for (const x of arr) if (Array.isArray(x)) scanLayout(x, depth + 1);
  }
  for (let pi = 0; pi < project.length; ++pi) scanLayout(project[pi], 0);

  lines.push("", `# layout instances mentioning t501: ${inst501.length}`);
  const spriteNames = new Set();
  function collectSprites(n) {
    if (typeof n === "string" && n.includes("-sheet")) spriteNames.add(n);
    else if (Array.isArray(n)) n.forEach(collectSprites);
  }
  inst501.slice(0, 50).forEach((inst) => collectSprites(inst));
  lines.push("sprites_from_layout:", ...[...spriteNames].sort().slice(0, 80));

  // Object type 501 default animation / image from project object types (index 4 in some exports)
  const objTypes = project[4];
  if (Array.isArray(objTypes)) {
    for (const ot of objTypes) {
      if (!Array.isArray(ot)) continue;
      if (ot[0] === 501 || ot[1] === "t501") {
        lines.push("", "## object_type_501", JSON.stringify(ot).slice(0, 3000));
      }
    }
  }

  fs.writeFileSync(path.join(outDir, "create_map_element_decode.txt"), lines.join("\n"));
  console.log("create_map_element_decode.txt", map.size, "tile keys");
}

// --- 2) Mag capacity: player_weapons (909) IV17 at spawn / Mag_reload ---
function decodeMagCapacity() {
  const byWeapon = {};
  walk(project[6], (e) => {
    const js = JSON.stringify(e);
    if (!js.includes("909") && !js.includes("player_weapons")) return;
    // SetInstanceVar IV17 on 909 with literal
    const m = js.match(/SetInstanceVar.*?"IV17",(\d+)\]/g);
    if (!m) return;
    // weapon item id from IV15 or nearby 151 reference
    const idm = js.match(/\[0,(\d{3})\]/g);
    if (!m.length) return;
    for (const line of m) {
      const cap = +line.match(/IV17",(\d+)/)[1];
      if (cap < 1 || cap > 100) continue;
      // try find weapon id 151-210 in same event
      const ids = [...js.matchAll(/CompareInstanceVar.*?"IV15".*?\[0,(\d+)\]/g)].map((x) => +x[1]);
      if (!ids.length) {
        const ids2 = [...js.matchAll(/\[0,(1[5-9]\d|2[0-1]\d)\]/g)].map((x) => +x[1]);
        if (ids2.length) ids.push(...ids2);
      }
      for (const wid of [...new Set(ids)]) {
        if (wid < 151 || wid > 210) continue;
        if (!byWeapon[wid]) byWeapon[wid] = {};
        byWeapon[wid][cap] = (byWeapon[wid][cap] || 0) + 1;
      }
    }
  });

  // Mag_reload: param [13, N, ...] might be reload seconds; look for IV17 on 909 in Mag_reload trees
  walk(project[6], (e) => {
    const js = JSON.stringify(e);
    if (!js.includes('"Mag_reload"')) return;
    const caps = [...js.matchAll(/"IV17",(\d+)/g)].map((x) => +x[1]);
    const wids = [...js.matchAll(/IV15.*?,\[0,(\d+)\]/g)].map((x) => +x[1]);
    for (const wid of wids) {
      if (wid < 151 || wid > 210) continue;
      for (const cap of caps) {
        if (cap < 1 || cap > 100) continue;
        if (!byWeapon[wid]) byWeapon[wid] = {};
        byWeapon[wid][cap] = (byWeapon[wid][cap] || 0) + 1;
      }
    }
  });

  const lines = ["# Mag capacity hints (player_weapons IV17)", "# Only use top count per weapon for sverka", ""];
  for (const wid of Object.keys(byWeapon).sort((a, b) => +a - +b)) {
    const top = Object.entries(byWeapon[wid]).sort((a, b) => b[1] - a[1])[0];
    lines.push(`${wid}: IV17=${top[0]} (hits ${top[1]}) all=${JSON.stringify(byWeapon[wid])}`);
  }
  fs.writeFileSync(path.join(outDir, "mag_capacity_decode.txt"), lines.join("\n"));
  console.log("mag_capacity_decode.txt weapons", Object.keys(byWeapon).length);
}

// --- 3) Drivable vehicles fuel / truck respawn ---
function decodeVehicles() {
  const fuelEvents = [];
  walk(project[6], (e) => {
    const js = JSON.stringify(e);
    if (!js.includes("284")) return;
    if (!js.includes("946") && !js.includes("Vehicle") && !js.includes("boat")) return;
    if (js.includes("SubInstanceVar") || js.includes("AddInstanceVar") || js.includes("Every")) {
      if (js.length < 6000) fuelEvents.push(js);
    }
  });
  fuelEvents.sort((a, b) => a.length - b.length);

  const truckHits = [];
  walk(project[6], (e) => {
    const js = JSON.stringify(e);
    if (js.includes("truck") || js.includes("Spawn_truck") || js.includes("Car_respawn_truck")) {
      if (js.length < 4000) truckHits.push(js.slice(0, 1500));
    }
  });

  const lines = [
    "# Drivable vehicles fuel (IV284) events",
    `fuel_event_samples: ${fuelEvents.length}`,
    "",
    ...fuelEvents.slice(0, 8).map((s, i) => `## sample ${i} len=${s.length}\n${s.slice(0, 1200)}`),
    "",
    "# Truck respawn strings",
    ...truckHits.slice(0, 15),
  ];
  fs.writeFileSync(path.join(outDir, "vehicle_fuel_decode.txt"), lines.join("\n"));
  console.log("vehicle_fuel_decode.txt");
}

// --- 4) Bullets_dmg_deal IV2 hit zones vs obj172 ---
function decodeHitZones() {
  const snippets = [];
  walk(project[6], (e) => {
    const js = JSON.stringify(e);
    if (!js.includes("Bullets_dmg_deal") && !js.includes("172")) return;
    if (!js.includes("IV2")) return;
    if (!js.includes("941") && !js.includes("172")) return;
    if (js.includes("CompareInstanceVar") && js.includes("IV2")) {
      if (js.length < 8000) snippets.push(js);
    }
  });
  snippets.sort((a, b) => a.length - b.length);

  // Parse dmg expressions near IV2 1/2/3
  const zoneMul = { 1: [], 2: [], 3: [] };
  for (const s of snippets) {
    for (const z of [1, 2, 3]) {
      if (!s.includes(`"IV2",[8,0],${z}`)) continue;
      const idx = s.indexOf(`"IV2",[8,0],${z}`);
      const chunk = s.slice(idx, idx + 2500);
      const muls = [...chunk.matchAll(/\[7,\[0,([\d.]+)\]\]/g)].map((m) => m[1]);
      const subs = [...chunk.matchAll(/SubInstanceVar.*?"IV0",([^"]+)/g)].map((m) => m[1].slice(0, 80));
      zoneMul[z].push({ muls: muls.slice(0, 10), subs: subs.slice(0, 5) });
    }
  }

  const lines = [
    "# Bullets_dmg_deal player hit zones (bullet IV2 1/2/3 vs obj172/obj941)",
    `snippet_count: ${snippets.length}`,
    "",
  ];
  for (const z of [1, 2, 3]) {
    lines.push(`## zone IV2=${z} samples ${zoneMul[z].length}`);
    zoneMul[z].slice(0, 3).forEach((s, i) => {
      lines.push(`  sample${i}: muls=${JSON.stringify(s.muls)} subs=${JSON.stringify(s.subs)}`);
    });
  }
  if (snippets[0]) lines.push("", "## smallest snippet", snippets[0].slice(0, 3500));

  fs.writeFileSync(path.join(outDir, "bullet_hit_zones_decode.txt"), lines.join("\n"));
  console.log("bullet_hit_zones_decode.txt");
}

decodeCreateMapElement();
decodeMagCapacity();
decodeVehicles();
decodeHitZones();
