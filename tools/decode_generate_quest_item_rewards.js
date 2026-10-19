/** Parse Generate_quest_item_impl.json: Q_target_sid → Q_reward_id random pool. */
const fs = require("fs");
const path = require("path");

function flattenJsonFile(p) {
  return JSON.stringify(JSON.parse(fs.readFileSync(p, "utf8")));
}

const j = flattenJsonFile(
  path.join(__dirname, "..", "cpp", "docs_gen", "functions", "Generate_quest_item_impl.json")
);

const by = {};
const blockRe = /"Q_target_sid"[\s\S]{0,160}?\[0,(\d+)\][\s\S]{0,1200}?"Q_reward_id"[\s\S]{0,220}?\[19,82,(\[[\s\S]*?\])\]/g;
let x;
while ((x = blockRe.exec(j)) !== null) {
  const sid = parseInt(x[1], 10);
  const pool = [...x[2].matchAll(/\[0,(\d+)\]/g)].map((y) => parseInt(y[1], 10));
  if (pool.length) by[sid] = pool;
}

const jb = flattenJsonFile(
  path.join(__dirname, "..", "cpp", "docs_gen", "functions", "Generate_quest_bandits_impl.json")
);
const mb = /"Q_reward_id"[\s\S]{0,220}?\[19,82,(\[[\s\S]*?\])\]/.exec(jb);
let banditPool = [];
if (mb) {
  banditPool = [...mb[1].matchAll(/\[0,(\d+)\]/g)].map((y) => parseInt(y[1], 10));
}

const outPath = path.join(__dirname, "..", "cpp", "docs_gen", "quest_item_reward_pools.json");
fs.writeFileSync(
  outPath,
  JSON.stringify(
    {
      _source: "Generate_quest_item / Generate_quest_bandits",
      by_target_sid: by,
      bandit_and_case_reward_pool: banditPool,
    },
    null,
    2
  )
);
console.log("item targets", Object.keys(by).length);
for (const [k, v] of Object.entries(by)) console.log(k, v.join(","));
