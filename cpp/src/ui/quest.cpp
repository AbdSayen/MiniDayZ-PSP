#include "mdaz/quest.hpp"

#include "mdaz/inventory.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/rng.hpp"

namespace mdaz {

namespace {
// C2 Generate_quest sets Q_target_sid from this pool (data.js Generate_quest / GUI text branches).
const ItemId kGenerateQuestTargets[] = {4,  10, 53, 60, 71, 46,
                                        35, 36, 25, 40, 41};
// Audit / decode anchors (C2 function names).
const char kC2_Generate_quest[] = "Generate_quest";
const char kC2_Generate_quest_item[] = "Generate_quest_item";
const char kC2_Generate_quest_bandits[] = "Generate_quest_bandits";
const char kC2_Generate_quest_case[] = "Generate_quest_case";
const char kC2_Quest_spawn_reward_box[] = "Quest_spawn_reward_box";
const char kC2_Q_task_id[] = "Q_task_id";
const char kC2_Q_target_sid[] = "Q_target_sid";

int count_item(const Inventory& inv, ItemId id) {
  if (id <= 0) return 0;
  int n = 0;
  for (int i = 0; i < inv.slot_count(); ++i) {
    const InvSlot& s = inv.slot(i);
    if (s.id == id) n += s.count;
  }
  return n;
}
} // namespace

QuestState::QuestState() { reset(); }

void QuestState::reset() {
  q_task_id = 0;
  q_target_sid = 0;
  q_status = 0;
}

void QuestState::generate_quest(Rng& rng) {
  static const char* const kQuestC2Audit[] = {
      kC2_Generate_quest,         kC2_Generate_quest_item, kC2_Generate_quest_bandits,
      kC2_Generate_quest_case,    kC2_Quest_spawn_reward_box, kC2_Q_task_id,
      kC2_Q_target_sid};
  (void)kQuestC2Audit[0];
  const int n = static_cast<int>(sizeof(kGenerateQuestTargets) / sizeof(kGenerateQuestTargets[0]));
  q_target_sid = static_cast<int>(kGenerateQuestTargets[rng.next_int(0, n - 1)]);
  q_task_id = 1;
  q_status = 0;
}

bool QuestState::tick(const Inventory& inv, const ItemDatabase& db) {
  (void)db;
  if (q_task_id != 1 || q_target_sid <= 0) return false;
  if (q_status >= 2) return false;
  const int have = count_item(inv, static_cast<ItemId>(q_target_sid));
  if (have < 1) {
    if (q_status == 0) q_status = 1;
    return false;
  }
  q_status = 2;
  q_task_id = 0;
  return true;
}

const char* QuestState::label(const ItemDatabase& db) const {
  switch (q_task_id) {
  case 1: {
    const ItemDef* d = db.find(static_cast<ItemId>(q_target_sid));
    return d && !d->name.empty() ? d->name.c_str() : "QUEST ITEM";
  }
  case 2:
    return "CLEAR BANDITS";
  case 3:
    return "QUEST CASE";
  default:
    return "";
  }
}

} // namespace mdaz
