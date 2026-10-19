#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

class ItemDatabase;
class Inventory;
class MapGenerator;
class Rng;
class LootSystem;

// C2 quest globals: Q_task_id, Q_target_sid, Q_status, Q_reward_id (+ Generate_quest_*).
struct QuestState {
  int q_task_id;    // 0 idle, 1 fetch item, 2 bandits, 3 quest case
  int q_target_sid; // item id for task 1 / 88 for task 3
  int q_status;     // 0 active, 1 in progress, 2 done (await reward), 3 canceled
  int q_reward_id;  // C2 Q_reward_id → Quest_spawn_reward_box branch
  Vec2 q_anchor;    // task 2 bandit camp POI
  bool map_pin;     // C2 Pin — show quest anchor on minimap
  int karma;        // C2 Karma global (cancel applies karma_points)

  QuestState();
  void reset();
  void generate_quest(Rng& rng, const MapGenerator& map, int current_level = 1);
  void generate_quest_item(Rng& rng);
  void generate_quest_bandits(Rng& rng, const class MapGenerator& map);
  void generate_quest_case();
  // Returns true when task just completed (q_status → 2, task id retained until finish_reward).
  bool tick(const Inventory& inv, const ItemDatabase& db, int bandits_alive_near_anchor = 0);
  // C2 Quest_spawn_reward_box — ground pile at pos (items_on_ground).
  void spawn_reward_box(LootSystem& loot, Vec2 pos) const;
  // After reward spawned: idle quest + roll next (caller runs generate_quest).
  void finish_reward();
  // C2 Quest_canceled + Karma(karma_points) from quest pad cancel path.
  bool cancel();
  bool awaiting_reward() const { return q_status == 2 && q_task_id > 0; }
  bool show_panel() const { return q_task_id > 0 && q_status < 2; }
  const char* label(const ItemDatabase& db) const;
  Vec2 map_pin_pos() const { return map_pin ? q_anchor : Vec2(0, 0); }
};

// l_eng_ui.xml task pad strings (ids 371–379) for compact HUD panel.
int quest_ui_intro_string_id(int q_task_id);
const char* quest_hud_line(const QuestState& q, const ItemDatabase& db);

// Verified Q_reward_id → l_eng item sid (tools/decode_quest_reward_box.js).
ItemId quest_reward_item_id(int q_reward_id);

} // namespace mdaz
