#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class ItemDatabase;
class Inventory;
class Rng;

// C2 quest globals: Q_task_id, Q_target_sid, Q_status (+ Generate_quest / Generate_quest_item).
// Full Generate_quest_* event tree not ported; task 1 = fetch Q_target_sid item (check_item_exists).
struct QuestState {
  int q_task_id;    // C2 Q_task_id (0 idle, 1 fetch item, 2 bandits, 3 quest case — partial)
  int q_target_sid; // C2 Q_target_sid (item id for task 1)
  int q_status;     // C2 Q_status (0 active panel, 1 in progress, 2 done/canceled stub)

  QuestState();
  void reset();
  // C2 Generate_quest on Q_task_id==0: pick target from verified sid list, set task 1.
  void generate_quest(Rng& rng);
  // Returns true when task just completed (task 1: item in inventory).
  bool tick(const Inventory& inv, const ItemDatabase& db);
  const char* label(const ItemDatabase& db) const;
  bool show_panel() const { return q_task_id > 0 && q_status < 2; }
};

} // namespace mdaz
