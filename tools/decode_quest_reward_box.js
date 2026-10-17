/** Decode Q_reward_id → Spawn_drop object index from Quest UI events in data.js. */
const fs = require("fs");
const path = require("path");

const dataPath = path.join(__dirname, "..", "original_docs", "data.js");
const s = fs.readFileSync(dataPath, "utf8").replace(/^\uFEFF/, "");

// C2: IF Q_reward_id = N → Spawn_drop on obj533 with [[0,itemSid]]
const re =
  /\[\[11,"Q_reward_id"\],\[8,0\],\[7,\[0,(\d+)\]\]\][\s\S]{0,400}?\[\[0,(\d+)\]\]/g;
const rewards = [];
let m;
while ((m = re.exec(s)) !== null) {
  rewards.push({ q_reward_id: parseInt(m[1], 10), item_id: parseInt(m[2], 10) });
}

const uniq = [];
const seen = new Set();
for (const r of rewards) {
  const k = r.q_reward_id;
  if (seen.has(k)) continue;
  seen.add(k);
  uniq.push(r);
}
uniq.sort((a, b) => a.q_reward_id - b.q_reward_id);

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "quest_reward_table.json");
fs.writeFileSync(
  outPath,
  JSON.stringify(
    {
      _source: "original_docs/data.js Q_reward_id branches (Spawn_drop [[0,sid]])",
      rewards: uniq,
    },
    null,
    2
  )
);
console.log("wrote", outPath, "entries", uniq.length);
console.log(JSON.stringify(uniq, null, 2));
