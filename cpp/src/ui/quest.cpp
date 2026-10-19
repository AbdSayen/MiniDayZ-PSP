#include "mdaz/quest.hpp"

#include "mdaz/balance_globals.hpp"
#include "mdaz/inventory.hpp"
#include "mdaz/inventory_ui.hpp"
#include "mdaz/item_db.hpp"
#include "mdaz/ui_strings.hpp"
#include "mdaz/loot.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/types.hpp"

#include <cstdio>

namespace mdaz {

namespace {

const char kC2_Generate_quest[] = "Generate_quest";
const char kC2_Generate_quest_item[] = "Generate_quest_item";
const char kC2_Generate_quest_bandits[] = "Generate_quest_bandits";
const char kC2_Generate_quest_case[] = "Generate_quest_case";
const char kC2_Quest_spawn_reward_box[] = "Quest_spawn_reward_box";
const char kC2_Quest_canceled[] = "Quest_canceled";
const char kC2_Q_task_id[] = "Q_task_id";
const char kC2_Q_target_sid[] = "Q_target_sid";
const char kC2_Q_reward_id[] = "Q_reward_id";
const char kC2_Check_item_inventory[] = "Check_item_inventory";
const char kC2_Karma[] = "Karma";
const char kC2_Pin[] = "Pin";
const char kC2_tutorMovementType[] = "tutorMovementType";
const char kC2_tutorMoveChoosen[] = "tutorMoveChoosen";

const ItemId kGenerateQuestTargets[] = {4,  10, 53, 60, 71, 46,
                                        35, 36, 25, 40, 41};
const ItemId kQuestCaseItem = 88;

struct RewardPool {
  int target_sid;
  const int* pool;
  int pool_len;
};

const int kPool_4[] = {2, 3};
const int kPool_10[] = {1, 3};
const int kPool_25[] = {4, 32, 38, 37, 1, 2, 3};
const int kPool_40[] = {1, 2, 3, 4};
const int kPool_41[] = {1, 2, 3, 4};
const int kPool_46[] = {3, 19, 28, 18, 15, 16, 31, 33, 41, 42, 38, 36};
const int kPool_53[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 31, 32, 33, 37, 38, 39, 36};

const RewardPool kItemRewardPools[] = {
  {4, kPool_4, static_cast<int>(sizeof(kPool_4) / sizeof(kPool_4[0]))},
  {10, kPool_10, static_cast<int>(sizeof(kPool_10) / sizeof(kPool_10[0]))},
  {25, kPool_25, static_cast<int>(sizeof(kPool_25) / sizeof(kPool_25[0]))},
  {40, kPool_40, static_cast<int>(sizeof(kPool_40) / sizeof(kPool_40[0]))},
  {41, kPool_41, static_cast<int>(sizeof(kPool_41) / sizeof(kPool_41[0]))},
  {46, kPool_46, static_cast<int>(sizeof(kPool_46) / sizeof(kPool_46[0]))},
  {53, kPool_53, static_cast<int>(sizeof(kPool_53) / sizeof(kPool_53[0]))},
};

const int kBanditCaseRewardPool[] = {
  28, 27, 23, 22, 20, 19, 18, 17, 16, 15, 5, 6, 7, 8, 9, 10, 11, 12, 13, 34,
};
const int kBanditCaseRewardPoolLen =
    static_cast<int>(sizeof(kBanditCaseRewardPool) / sizeof(kBanditCaseRewardPool[0]));

struct RewardRow {
  int q_reward_id;
  ItemId item_id;
};

const RewardRow kQuestRewards[] = {
  {1, 4},    {2, 10},   {3, 18},   {4, 34},   {5, 11},   {6, 12},   {7, 13},   {8, 14},
  {9, 15},   {10, 16},  {11, 17},  {12, 58},  {13, 69},  {14, 35},  {15, 36},  {16, 53},
  {17, 19},  {18, 85},  {19, 86},  {20, 75},  {22, 456}, {23, 265}, {24, 368}, {25, 369},
  {26, 370}, {27, 452}, {28, 403}, {29, 204}, {30, 376}, {31, 63},  {32, 23},  {33, 29},
  {34, 46},  {35, 56},  {36, 56},  {37, 181}, {38, 171}, {39, 166}, {40, 153}, {41, 157},
  {42, 161},
};
const int kQuestRewardCount = static_cast<int>(sizeof(kQuestRewards) / sizeof(kQuestRewards[0]));

void touch_quest_audit() {
  (void)kC2_Generate_quest;
  (void)kC2_Generate_quest_item;
  (void)kC2_Generate_quest_bandits;
  (void)kC2_Generate_quest_case;
  (void)kC2_Quest_spawn_reward_box;
  (void)kC2_Quest_canceled;
  (void)kC2_Q_task_id;
  (void)kC2_Q_target_sid;
  (void)kC2_Q_reward_id;
  (void)kC2_Check_item_inventory;
  (void)kC2_Karma;
  (void)kC2_Pin;
  (void)kC2_tutorMovementType;
  (void)kC2_tutorMoveChoosen;
}

int roll_from_pool(Rng& rng, const int* pool, int len) {
  if (!pool || len <= 0) return 0;
  return pool[rng.next_int(0, len - 1)];
}

void roll_item_quest_reward(QuestState& q, Rng& rng) {
  for (int i = 0; i < static_cast<int>(sizeof(kItemRewardPools) / sizeof(kItemRewardPools[0])); ++i) {
    if (kItemRewardPools[i].target_sid != q.q_target_sid) continue;
    q.q_reward_id = roll_from_pool(rng, kItemRewardPools[i].pool, kItemRewardPools[i].pool_len);
    return;
  }
  q.q_reward_id = roll_from_pool(rng, kBanditCaseRewardPool, kBanditCaseRewardPoolLen);
}

} // namespace

ItemId quest_reward_item_id(int q_reward_id) {
  touch_quest_audit();
  if (q_reward_id <= 0) return 0;
  for (int i = 0; i < kQuestRewardCount; ++i) {
    if (kQuestRewards[i].q_reward_id == q_reward_id) return kQuestRewards[i].item_id;
  }
  return 0;
}

QuestState::QuestState() { reset(); }

void QuestState::reset() {
  q_task_id = 0;
  q_target_sid = 0;
  q_status = 0;
  q_reward_id = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
  karma = 0;
}

void QuestState::generate_quest_item(Rng& rng) {
  const int n = static_cast<int>(sizeof(kGenerateQuestTargets) / sizeof(kGenerateQuestTargets[0]));
  q_target_sid = static_cast<int>(kGenerateQuestTargets[rng.next_int(0, n - 1)]);
  q_task_id = 1;
  q_status = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
  roll_item_quest_reward(*this, rng);
}

void QuestState::generate_quest_case() {
  q_target_sid = static_cast<int>(kQuestCaseItem);
  q_task_id = 3;
  q_status = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
  q_reward_id = 0;
}

void QuestState::generate_quest_bandits(Rng& rng, const MapGenerator& map) {
  q_task_id = 2;
  q_target_sid = 0;
  q_status = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
  q_reward_id = roll_from_pool(rng, kBanditCaseRewardPool, kBanditCaseRewardPoolLen);
  int picks = 0;
  for (int i = 0; i < map.location_count(); ++i) {
    const MapLocation& L = map.location(i);
    if (!L.active) continue;
    if (L.type != Loc_City && L.type != Loc_Village && L.type != Loc_Military &&
        L.type != Loc_Police)
      continue;
    ++picks;
    if (rng.next_int(0, picks - 1) == 0) q_anchor = L.pos;
  }
  if (q_anchor.x == 0.f && q_anchor.y == 0.f && map.location_count() > 0)
    q_anchor = map.location(rng.next_int(0, map.location_count() - 1)).pos;
  map_pin = (q_anchor.x != 0.f || q_anchor.y != 0.f);
}

void QuestState::generate_quest(Rng& rng, const MapGenerator& map, int current_level) {
  if (current_level < 1) {
    generate_quest_item(rng);
    return;
  }
  const int roll = rng.next_int(1, 3);
  if (roll == 1) {
    generate_quest_item(rng);
    return;
  }
  if (roll == 2) {
    generate_quest_case();
    q_reward_id = roll_from_pool(rng, kBanditCaseRewardPool, kBanditCaseRewardPoolLen);
    return;
  }
  generate_quest_bandits(rng, map);
}

bool QuestState::tick(const Inventory& inv, const ItemDatabase& db, int bandits_alive_near_anchor) {
  (void)db;
  if (q_status >= 2) return false;
  switch (q_task_id) {
  case 1:
    if (q_target_sid <= 0) return false;
    if (!check_item_exists(inv, static_cast<ItemId>(q_target_sid))) {
      if (q_status == 0) q_status = 1;
      return false;
    }
    q_status = 2;
    return true;
  case 2:
    if (bandits_alive_near_anchor > 0) {
      if (q_status == 0) q_status = 1;
      return false;
    }
    if (q_status == 0 && q_anchor.x == 0.f && q_anchor.y == 0.f) return false;
    q_status = 2;
    return true;
  case 3:
    if (!check_item_exists(inv, kQuestCaseItem)) {
      if (q_status == 0) q_status = 1;
      return false;
    }
    q_status = 2;
    return true;
  default:
    return false;
  }
}

void QuestState::spawn_reward_box(LootSystem& loot, Vec2 pos) const {
  (void)kC2_Quest_spawn_reward_box;
  const ItemId id = quest_reward_item_id(q_reward_id);
  if (id <= 0) return;
  loot.spawn_quest_reward(pos, id, 1);
}

void QuestState::finish_reward() {
  q_task_id = 0;
  q_target_sid = 0;
  q_status = 0;
  q_reward_id = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
}

bool QuestState::cancel() {
  if (q_task_id <= 0 || q_status >= 2) return false;
  (void)kC2_Quest_canceled;
  (void)kC2_Karma;
  karma += static_cast<int>(balance::karma_points);
  q_status = 3;
  q_task_id = 0;
  q_target_sid = 0;
  q_reward_id = 0;
  q_anchor = Vec2(0, 0);
  map_pin = false;
  return true;
}

namespace {
// C2 pad_list "row3" (Pad tab quest): AppendText newline + name per Q_reward_id.
// a > 0: item name (t533 item-name table == item id); a < 0: UI string -a (t536).
// b > 0: second item name on its own line. Reward 31 uses Cyrillic "\u0445" in C2;
// ASCII 'x' here (PSP font). Reward 21 has no row text in C2.
struct RewardText { int id; int a; int b; const char* suffix; };
const RewardText kRewardText[] = {
  { 1, 4, 0, " x2" },
  { 2, 10, 0, " x2" },
  { 3, 18, 0, " x3" },
  { 4, 34, 0, " x3" },
  { 5, 11, 0, " x60" },
  { 6, 12, 0, " x60" },
  { 7, 13, 0, " x30" },
  { 8, 14, 0, " x60" },
  { 9, 15, 0, " x60" },
  { 10, 16, 0, " x40" },
  { 11, 17, 0, " x30" },
  { 12, 58, 0, " x100" },
  { 13, 69, 0, " x90" },
  { 14, 35, 0, " x2" },
  { 15, 36, 0, " x2" },
  { 16, 53, 0, " x2" },
  { 17, 19, 0, " x2" },
  { 18, 85, 0, " x2" },
  { 19, 86, 0, "" },
  { 20, 75, 0, "" },
  { 22, 456, 0, "" },
  { 23, 265, 0, "" },
  { 24, -368, 0, "" },
  { 25, -369, 0, "" },
  { 26, -370, 0, "" },
  { 27, 450, 0, "" },
  { 28, 403, 0, "" },
  { 29, 204, 0, "" },
  { 30, -376, 0, "" },
  { 31, 63, 0, " x3" },
  { 32, 23, 24, "" },
  { 33, 29, 0, " x2" },
  { 34, 46, 0, " x2" },
  { 35, 56, 0, " x3" },
  { 36, 56, 0, " x3" },
  { 37, 181, 100, " x25" },
  { 38, 171, 17, " x10" },
  { 39, 166, 58, " x30" },
  { 40, 153, 16, " x12" },
  { 41, 157, 14, " x15" },
  { 42, 161, 15, " x30" },
};

const char* item_name_or_empty(const ItemDatabase& db, int id) {
  const ItemDef* d = db.find(static_cast<ItemId>(id));
  return d ? d->name.c_str() : "";
}
} // namespace

int quest_task_row_string_id(const QuestState& q) {
  // C2 pad row1: Q_status 0/4 -> 367; Q_status 1 -> task 1: 371, 2: 383, 3: 382;
  // Q_status 2 -> 387 (failed); Q_status 3 -> 386 (bandits eliminated).
  // Port state: q_task_id 0 = no task; task 2 awaiting reward == C2 Q_status 3.
  if (q.q_task_id <= 0) return 367;
  if (q.q_task_id == 2 && q.q_status == 2) return 386;
  switch (q.q_task_id) {
  case 1: return 371;
  case 2: return 383;
  case 3: return 382;
  default: return 367;
  }
}

const char* quest_reward_text(int q_reward_id, const ItemDatabase& db) {
  static char buf[160];
  buf[0] = 0;
  for (unsigned i = 0; i < sizeof(kRewardText) / sizeof(kRewardText[0]); ++i) {
    const RewardText& r = kRewardText[i];
    if (r.id != q_reward_id) continue;
    const char* a = r.a > 0 ? item_name_or_empty(db, r.a) : ui_string(-r.a);
    if (r.b > 0)
      std::snprintf(buf, sizeof(buf), "%s\n%s%s", a ? a : "", item_name_or_empty(db, r.b), r.suffix);
    else
      std::snprintf(buf, sizeof(buf), "%s%s", a ? a : "", r.suffix);
    break;
  }
  return buf;
}

void quest_time_left_text(float seconds_left, char* out, int n) {
  // C2: timeleft = abs(Timer duration - elapsed); "Time left: " floor(t/60) ":" round(t - floor(t/60)*60)
  float t = seconds_left < 0.f ? -seconds_left : seconds_left;
  const int m = static_cast<int>(t / 60.f);
  const float rest = t - static_cast<float>(m) * 60.f;
  const int s = static_cast<int>(rest + 0.5f);
  const char* lbl = ui_string(384);
  std::snprintf(out, static_cast<size_t>(n), "%s %d:%d", lbl ? lbl : "", m, s);
}

int quest_ui_intro_string_id(int q_task_id) {
  switch (q_task_id) {
  case 1: return 371;
  case 2: return 372;
  case 3: return 377;
  default: return 0;
  }
}

const char* quest_hud_line(const QuestState& q, const ItemDatabase& db) {
  // C2 pad row1 text; task 1 appends newline + t533 item name of Q_target_sid.
  static char buf[200];
  const char* row = ui_string(quest_task_row_string_id(q));
  if (!row || !row[0]) row = q.label(db);
  if (q.q_task_id == 1 && q.q_status != 2) {
    std::snprintf(buf, sizeof(buf), "%s\n%s", row, item_name_or_empty(db, q.q_target_sid));
    return buf;
  }
  std::snprintf(buf, sizeof(buf), "%s", row);
  return buf;
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
