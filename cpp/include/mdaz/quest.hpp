#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class ItemDatabase;
class Inventory;
class MapGenerator;
class Rng;

// C2 quest globals: Q_task_id, Q_target_sid, Q_status (+ Generate_quest / Generate_quest_item).
// Full Generate_quest_* event tree not ported; task 1 = fetch Q_target_sid item (check_item_exists).
struct QuestState {
  int q_task_id;    // C2 Q_task_id (0 idle, 1 fetch item, 2 bandits, 3 quest case — partial)
  int q_target_sid; // C2 Q_target_sid (item id for task 1)
  int q_status;     // C2 Q_status (0 active panel, 1 in progress, 2 done/canceled stub)
  Vec2 q_anchor;    // task 2: bandit camp POI (Spawn_bandit_camp anchor)

  QuestState();
  void reset();
  // C2 Generate_quest (CurrentLevel>=1): random branch → item / bandits / case.
  void generate_quest(Rng& rng, const MapGenerator& map, int current_level = 1);
  void generate_quest_item(Rng& rng);
  void generate_quest_bandits(Rng& rng, const class MapGenerator& map);
  void generate_quest_case();
  // Returns true when task just completed (task 1: item in inventory).
  bool tick(const Inventory& inv, const ItemDatabase& db, int bandits_alive_near_anchor = 0);
  const char* label(const ItemDatabase& db) const;
  bool show_panel() const { return q_task_id > 0 && q_status < 2; }
};

} // namespace mdaz
