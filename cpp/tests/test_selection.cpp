// Selected-slot regression tests (bug 2026-10-19: USE consumed the FIRST item
// instead of the selected hotbar / inventory slot). Each check drives the real
// Game methods through GameTestAccess (friend of mdaz::Game).
#include "mdaz/game.hpp"
#include "mdaz/craft.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/platform_null.hpp"
#include "mdaz/combat.hpp"
#include "mdaz/rng.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/map_decode_tables.hpp"

#include <cstdio>
#include <new>
#include <string>

namespace mdaz {
struct GameTestAccess {
  static Inventory& inv(Game& g) { return g.inventory_; }
  static ItemDatabase& db(Game& g) { return g.items_; }
  static SurvivalState& surv(Game& g) { return g.player_.survival; }
  static int& eq(Game& g) { return g.equipped_slot_; }
  static int& mag(Game& g) { return g.mag_rounds_; }
  static int& craft_idx(Game& g) { return g.craft_menu_index_; }
  static InventoryUiState& ui(Game& g) { return g.inventory_ui_; }
  static void use(Game& g) { g.use_equipped_item(); }
  static void use_bag(Game& g, int s) { g.use_bag_slot(s); }
  static void equip_bag(Game& g, int s) { g.equip_bag_slot(s); }
  static void drop(Game& g, int s) { g.try_drop_slot(s); }
  static void drop_eq(Game& g) { g.try_drop_equipped(); }
  static void reload(Game& g) { g.reload_cd_ = 0.f; g.gun_jammed_ = false; g.try_mag_reload(); }
  static void craft(Game& g) { g.try_craft_menu_recipe(); }
  static void craft_ui(Game& g, const InputState& in) { g.update_craft_menu(in); }
  static void inv_ui(Game& g, const InputState& in) { g.update_inventory_ui(in); }
  // 2026-10-19 queue: restored Game bodies + debug teleport + NPC forest block.
  static MapGenerator& map(Game& g) { return g.mapgen_; }
  static Player& pl(Game& g) { return g.player_; }
  static ZombieSystem& zeds(Game& g) { return g.zombies_; }
  static AnimalSystem& animals(Game& g) { return g.animals_; }
  static BanditSystem& bandits(Game& g) { return g.bandits_; }
  static LootSystem& loot(Game& g) { return g.loot_; }
  static WorldTilemaps& tiles(Game& g) { return g.world_tiles_; }
  static CharAnims& anims(Game& g) { return g.char_anims_; }
  static const DmgHintSystem& hints(Game& g) { return g.dmg_hints_; }
  static void hurt_z(Game& g, Zombie& z, float d) { g.hurt_zombie(z, d); }
  static void hurt_a(Game& g, Animal& a, float d) { g.hurt_animal(a, d); }
  static void hurt_b(Game& g, Bandit& b, float d) { g.hurt_bandit(b, d); }
  static bool block(Game& g, Vec2& p, Vec2 prev) { return g.npc_forest_block(p, prev); }
  static void pack(Game& g, LootSaveView& v) { g.pack_loot_view(v); }
  static void apply(Game& g, const LootSaveView& v) { g.apply_loot_view(v); }
  static int corpse_count(Game& g) { return g.corpse_count_; }
  static float corpse_life(Game& g, int i) { return g.corpses_[i].life; }
  static float corpse_die_t(Game& g, int i) { return g.corpses_[i].die_anim_t; }
  static const char* corpse_base(Game& g, int i) { return g.corpses_[i].anim_base; }
  // 2026-10-19 step 1: restored bodies.
  static void explode(Game& g, Vec2 p, float d, bool fire) { g.explode_at(p, d, fire); }
  static void burn_tick(Game& g, float dt) { g.tick_molotov_burn(dt); }
  static bool& in_bunker(Game& g) { return g.in_bunker_; }
  static void zcheck(Game& g) { g.zed_check_bunker(); }
  static void bspawn(Game& g, Vec2 p) { g.spawn_bunker_zeds(p); }
  static bool spawner_used(Game& g, int i) { return g.bunker_spawner_used_[i]; }
  static void notify(Game& g, ItemId id) { g.notify_weapon_unlock(id); }
  static float toast_t(Game& g) { return g.wpn_unlock_toast_t_; }
  static ItemId toast_id(Game& g) { return g.wpn_unlock_toast_id_; }
  static QuestState& quest(Game& g) { return g.quest_; }
  static void quest_done(Game& g, int task) { g.on_quest_completed(task); }
  static void quest_bandits(Game& g) { g.maybe_spawn_quest_bandits(); }
  static bool fence(Game& g, int v) { return g.try_build_fence(v); }
  static int trap_count(Game& g) { return g.trap_count_; }
  static float trap_life(Game& g, int i) { return g.traps_[i].life; }
  static int trap_kind(Game& g, int i) { return g.traps_[i].kind; }
  static void kill_z(Game& g, Zombie& z, ItemId w) { g.hurt_zombie(z, 1.0e6f, w); }
  static void stream(Game& g, LocationReloadMode m) { g.sync_location_stream(m); }
};
}

using namespace mdaz;
typedef GameTestAccess A;

static int g_fail = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "OK  " : "FAIL", what);
  if (!ok) ++g_fail;
}

static void reset(Game& g) {
  A::inv(g).clear();
  A::surv(g).food = 40.f; A::surv(g).water = 40.f; A::surv(g).blood = 60.f;
  A::eq(g) = 0; A::mag(g) = 0;
}
static void put(Game& g, int slot, ItemId id, int n) {
  A::inv(g).slot(slot).id = id; A::inv(g).slot(slot).count = n;
}


static int inv_count(Game& g, ItemId id) {
  int n = 0;
  for (int i = 0; i < A::inv(g).slot_count(); ++i)
    if (A::inv(g).slot(i).id == id) n += A::inv(g).slot(i).count;
  return n;
}

// 2026-10-19 step 1: Molotov_damage, Zed_check_bunker / Bunker_spawn_zeds, Show_unlock_wpn,
// quest completion + Generate_quest_bandits camp, build_fence.
static void test_restored_step1(Game& g) {
  A::zeds(g).clear();
  A::animals(g).clear();
  A::bandits(g).clear();
  // --- Molotov: no blast damage, 5 s burning, 10 dmg (tank 15) every 0.5 s, wolves yes, deer no.
  Zombie* z = A::zeds(g).spawn(Zed_Normal, Vec2(5000.f, 5000.f));
  Zombie* t = A::zeds(g).spawn(Zed_Tank, Vec2(5040.f, 5000.f));
  Zombie* far = A::zeds(g).spawn(Zed_Normal, Vec2(5200.f, 5000.f));
  Animal* w = A::animals(g).spawn(Animal_Wolf, Vec2(4980.f, 5010.f));
  Animal* d = A::animals(g).spawn(Animal_Deer, Vec2(5010.f, 4990.f));
  Bandit* b = A::bandits(g).spawn(Vec2(5000.f, 5050.f), false);
  const float zh = z->hp, th = t->hp, fh = far->hp, wh = w->hp, dh = d->hp, bh = b->hp;
  A::explode(g, Vec2(5000.f, 5000.f), 40.f, true);
  check(z->hp == zh && t->hp == th && w->hp == wh && b->hp == bh, "molotov: no blast damage");
  check(z->burn_t == 5.f && t->burn_t == 5.f && w->burn_t == 5.f && b->burn_t == 5.f,
        "molotov: zed/tank/wolf/bandit in light_sprite get burning 5 s");
  check(far->burn_t == 0.f && d->burn_t == 0.f, "molotov: far zed and deer do not burn");
  A::burn_tick(g, 0.5f);
  check(zh - z->hp == 10.f && th - t->hp == 15.f && wh - w->hp == 10.f && bh - b->hp == 10.f,
        "Molotov_damage tick: 10 (tank 15) per 0.5 s");
  for (int k = 0; k < 12; ++k) A::burn_tick(g, 0.5f);
  check(z->burn_t <= 0.f && far->hp == fh && d->hp == dh, "burning ends after 5 s");
  // --- Bunker spawners.
  A::zeds(g).clear();
  A::in_bunker(g) = true;
  const Vec2 sp(kBunkerZedSpawners[0].x, kBunkerZedSpawners[0].y);
  A::pl(g).pos = Vec2(sp.x + 300.f, sp.y); // 800..1500 C2 px = 213..400 port px
  A::zcheck(g);
  const int n1 = A::zeds(g).alive_count();
  check(A::spawner_used(g, 0) && n1 >= 1, "Zed_check_bunker spawns at a spawner 800-1500 away");
  A::zcheck(g);
  check(A::spawner_used(g, 0), "spawner boolean stays set");
  A::pl(g).pos = Vec2(sp.x + 10.f, sp.y);
  bool near_ok = true;
  { A::zeds(g).clear(); A::zcheck(g); for (Zombie* q = A::zeds(g).begin(); q != A::zeds(g).end(); ++q)
      if (q->alive && (q->pos.x - sp.x) * (q->pos.x - sp.x) + (q->pos.y - sp.y) * (q->pos.y - sp.y) < 60.f * 60.f) near_ok = false; }
  check(near_ok, "no spawn from a spawner closer than 800 C2 px");
  bool sizes_ok = true;
  for (int k = 0; k < 40; ++k) {
    A::zeds(g).clear();
    A::bspawn(g, Vec2(500.f, 300.f));
    const int n = A::zeds(g).alive_count();
    if (n != 5 && n != 7 && n != 3 && n != 1) sizes_ok = false;
  }
  check(sizes_ok, "Bunker_spawn_zeds groups are 5 fast / 7 army / 3 shooter / 1+4 / 1 tank");
  A::in_bunker(g) = false;
  A::zeds(g).clear();
  // --- Weapon unlock toast.
  A::notify(g, 186);
  check(A::toast_id(g) == 186 && A::toast_t(g) > 3.f, "notify_weapon_unlock shows the toast 3 s");
  // --- Quest completion + quest bandits.
  A::db(g).load_from_json_file("data/items.json");
  A::inv(g).clear();
  A::inv(g).add_item(1, 1, A::db(g));
  QuestState& q = A::quest(g);
  q.q_task_id = 1; q.q_target_sid = 1; q.q_status = 2;
  const int karma0 = q.karma;
  A::quest_done(g, 1);
  check(inv_count(g, 1) == 0 && q.karma == karma0 + 10, "quest item task: slot removed, Karma +10");
  q.q_task_id = 2; q.q_status = 0; q.q_anchor = Vec2(9000.f, 9000.f);
  A::bandits(g).clear();
  A::quest_bandits(g);
  const int nb = A::bandits(g).alive_count();
  A::quest_bandits(g);
  check(nb == 4 && A::bandits(g).alive_count() == 4, "Generate_quest_bandits camp spawned once");
  const int karma1 = q.karma;
  A::quest_done(g, 2);
  check(q.karma == karma1 + 20, "quest bandit task: Karma +20");
  // --- build_fence.
  A::inv(g).clear();
  A::inv(g).add_item(38, 6, A::db(g));
  A::inv(g).add_item(50, 1, A::db(g));
  const int tc = A::trap_count(g);
  const bool ok = A::fence(g, 1);
  check(ok && inv_count(g, 38) == 1 && inv_count(g, 50) == 0, "build_fence: -5 sticks, woodpile cleared");
  check(A::trap_count(g) == tc + 1 && A::trap_kind(g, tc) == 2 && A::trap_life(g, tc) > 1.0e8f,
        "fence placed, no lifetime");
  check(!A::fence(g, 2), "build_fence needs 5 sticks + woodpile");
  // --- Location stream (recovered sync_location_stream / spawn_loot_for_poi / spawn_hostiles_for_poi).
  MapGenerator& m = A::map(g);
  m.generate_new_game(140);
  int vi = -1;
  for (int i = 0; i < m.location_count(); ++i) if (m.location(i).type == Loc_Village) { vi = i; break; }
  const Vec2 vp = m.location(vi).pos;
  A::zeds(g).clear(); A::loot(g).clear();
  A::pl(g).pos = Vec2(vp.x + 1000.f, vp.y);
  A::stream(g, LocationReload_Start);
  const int loot1 = A::loot(g).count(), z1 = A::zeds(g).alive_count();
  check(m.location(vi).active && m.location(vi).loot_seeded && loot1 >= 1 && z1 >= 1,
        "Reload_locations_start creates a POI 1000 px away (loot + zeds)");
  bool far_inactive = true;
  for (int i = 0; i < m.location_count(); ++i) {
    const float dx = m.location(i).pos.x - A::pl(g).pos.x, dy = m.location(i).pos.y - A::pl(g).pos.y;
    if (dx * dx + dy * dy > 3100.f * 3100.f && m.location(i).active) far_inactive = false;
  }
  check(far_inactive, "start radius = respawn_radius2 + 1000 (nothing active beyond)");
  A::pl(g).pos = Vec2(vp.x + 2600.f, vp.y);
  A::stream(g, LocationReload_Runtime);
  check(!m.location(vi).active, "Reload_locations erases a location beyond respawn_radius2");
  A::pl(g).pos = Vec2(vp.x + 1000.f, vp.y);
  A::stream(g, LocationReload_Runtime);
  check(m.location(vi).active && A::loot(g).count() == loot1, "re-created location: loot not seeded twice");
}

int test_queue_q1() {
  std::printf("--- test_queue_q1 ---\n");
  g_fail = 0;
  Game* gp = new (std::nothrow) Game();
  if (!gp) { std::printf("FAIL alloc Game\n"); return 1; }
  Game& g = *gp;
  IPlatform* np = create_null_platform();
  A::anims(g).load(*np, "data/char_anims.txt");
  A::map(g).generate_new_game(140);
  int heli = 0;
  for (int i = 0; i < A::map(g).location_count(); ++i)
    if (A::map(g).location(i).type == Loc_Helicrash) ++heli;
  check(heli >= 1, "seed 140 map has helicrash locations");
  {
    // Cross-platform determinism: the POI list must not depend on argument evaluation order.
    // Replay the first draws by hand in x-then-y order (Rng::int_pair) and compare.
    std::printf("POI list seed 140 (%d), hash %08x:\n", A::map(g).location_count(), A::map(g).poi_hash());
    for (int i = 0; i < A::map(g).location_count(); ++i) {
      const MapLocation& L = A::map(g).location(i);
      std::printf("  #%02d type %2d sector (%2d,%2d) world (%.0f,%.0f)\n", i, (int)L.type,
                  L.sector_x, L.sector_y, L.pos.x, L.pos.y);
    }
    Rng r(140u);
    r.next_int(0, 1);                    // n_helis = 2 + next_int(0,1)
    const int ex = r.next_int(1, 14);    // first village: x drawn first
    const int ey = r.next_int(1, 14);
    check(A::map(g).location(0).sector_x == ex && A::map(g).location(0).sector_y == ey,
          "first POI = (x draw, y draw) in fixed order (compiler-independent)");
    MapGenerator m2;
    m2.generate_new_game(140);
    check(m2.poi_hash() == A::map(g).poi_hash(), "same seed -> same POI hash");
  }
  // Invented mechanics removed: zed kill spawns no crows; corpse life = Bot_corpse_stay_time.
  // Minimap cell values are C2 Array-150 markers only.
  bool cells_ok = true;
  for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
    const int v = A::map(g).sector_cell(x, y);
    if (!(v == 0 || v == 6 || v == 9 || v == 11 || (v >= 12 && v <= 15))) cells_ok = false;
  }
  check(cells_ok, "sector_cell values in {0,6,9,11,12..15}");
  // NPC forest block (tilemap_forest Solid, tile 4 = full box).
  A::tiles(g).stamp_fill("tilemap_forest", 3000.f, 3000.f, 2, 2, 4, 150, 150, 3);
  Vec2 p(3010.f, 2990.f);  // step into the forest from outside (north)
  const Vec2 prev(3010.f, 2950.f);
  Vec2 q = Vec2(3010.f, 3010.f);
  check(A::block(g, q, prev) && !A::tiles(g).forest_solid_at(q.x, q.y), "NPC step into forest is undone");
  Vec2 r = p;
  check(!A::block(g, r, prev) && r.x == p.x && r.y == p.y, "NPC outside forest moves freely");
  Vec2 in1(3100.f, 3100.f);
  check(!A::block(g, in1, Vec2(3090.f, 3090.f)), "NPC already inside is allowed to walk out");
  // hurt_*: NPC_get_damage + dmg_hint + kill + die anim.
  Zombie* z = A::zeds(g).spawn(Zed_Normal, Vec2(500.f, 500.f));
  check(z != 0, "spawn zed");
  if (z) {
    const float hp0 = z->hp;
    const int hints0 = A::hints(g).count();
    A::hurt_z(g, *z, 1.f);
    check(z->hp == hp0 - 1.f && z->alive, "hurt_zombie subtracts damage");
    check(A::hints(g).count() == hints0 + 1, "hurt_zombie spawns a dmg_hint");
    const int c0 = A::corpse_count(g);
    A::hurt_z(g, *z, 100000.f);
    check(!z->alive && z->hp == 0.f, "lethal hit kills zed (hp clamped to 0)");
    check(A::corpse_count(g) == c0 + 1, "zed corpse spawned");
    if (A::corpse_count(g) > c0) {
      check(A::corpse_die_t(g, c0) > 0.6f && A::corpse_die_t(g, c0) < 0.65f,
            "zed corpse plays die1/die2 (5 frames @ 8 fps = 0.625s)");
      check(std::string(A::corpse_base(g, c0)) == "zed_normal_skin1", "die anim base = zed_normal_skin1");
    }
  }
  if (true) {
    const int crows0 = A::animals(g).alive_of_type(Animal_Crow);
    for (int k = 0; k < 20; ++k) {
      Zombie* zz = A::zeds(g).spawn(Zed_Normal, Vec2(900.f + k, 900.f));
      if (zz) A::hurt_z(g, *zz, 100000.f);
    }
    check(A::animals(g).alive_of_type(Animal_Crow) == crows0, "zed kills spawn no crows (removed, not in data.js)");
    check(A::corpse_life(g, A::corpse_count(g) - 1) == (float)balance::Bot_corpse_stay_time,
          "zed corpse life = Bot_corpse_stay_time (1.15x removed)");
  }
  Animal* a = A::animals(g).spawn(Animal_Wolf, Vec2(600.f, 600.f));
  if (a) {
    A::hurt_a(g, *a, 100000.f);
    check(!a->alive, "hurt_animal kills wolf");
    check(A::corpse_die_t(g, A::corpse_count(g) - 1) > 0.37f, "wolf corpse plays die1 (3 frames @ 8 fps)");
  }
  Bandit* b = A::bandits(g).spawn(Vec2(700.f, 700.f), false);
  if (b) {
    const int k0 = A::pl(g).bandits_killed;
    A::hurt_b(g, *b, 100000.f);
    check(!b->alive && A::pl(g).bandits_killed == k0 + 1, "hurt_bandit kills + counts bandit");
  }
  // Loot save view round trip (C2 Save_loot / Load_loot).
  A::loot(g).clear();
  A::loot(g).spawn_item(Vec2(123.f, 456.f), 6, 3);
  A::loot(g).spawn_item(Vec2(10.f, 20.f), 42, 1);
  LootSaveView* v = new LootSaveView();
  A::pack(g, *v);
  check(v->count == 2, "pack_loot_view stores both piles");
  A::loot(g).clear();
  A::apply(g, *v);
  bool rt = A::loot(g).count() == 2 && A::loot(g).at(0)->pos.x == 123.f &&
            A::loot(g).at(0)->items[0] == 6 && A::loot(g).at(0)->counts[0] == 3;
  check(rt, "apply_loot_view restores position/item/count");
  LootSaveView empty;
  A::apply(g, empty);
  check(A::loot(g).count() == 2, "empty loot view (old save) keeps current loot");
  delete v;
  test_restored_step1(g);
  delete np;
  delete gp;
  std::printf("test_queue_q1: %d failure(s)\n", g_fail);
  return g_fail ? 1 : 0;
}

int test_selection() {
  std::printf("--- test_selection ---\n");
  g_fail = 0;
  Game* gp = new (std::nothrow) Game();
  if (!gp) { std::printf("FAIL alloc Game\n"); return 1; }
  Game& g = *gp;
  if (!A::db(g).load_from_json_file("data/items.json")) {
    std::printf("FAIL load data/items.json (run from cpp/)\n"); delete gp; return 1;
  }
  const ItemId kApple = 6, kPipsi = 8, kKnife = 42, kGas = 53;

  // 1. USE acts on the selected hotbar slot, not slot 0.
  reset(g); put(g, 0, kApple, 1); put(g, 2, kPipsi, 1); A::eq(g) = 2;
  A::use(g);
  check(A::inv(g).slot(2).empty() && A::inv(g).slot(0).id == kApple && A::inv(g).slot(0).count == 1,
        "USE consumes selected slot 2 (Pipsi), slot 0 Apple untouched");
  // 2. USE on an empty selected slot does nothing (no first-item fallback).
  reset(g); put(g, 0, kApple, 1); A::eq(g) = 5;
  const float food0 = A::surv(g).food;
  A::use(g);
  check(A::inv(g).slot(0).count == 1 && A::surv(g).food == food0, "USE on empty slot: no fallback to slot 0");
  // 3. USE on a non-consumable selected item does not eat another slot.
  reset(g); put(g, 0, kApple, 1); put(g, 3, kKnife, 1); A::eq(g) = 3;
  A::use(g);
  check(A::inv(g).slot(0).count == 1 && A::inv(g).slot(3).id == kKnife, "USE on knife: apple in slot 0 untouched");
  // 4. Gasoline elsewhere in the bag is not used when another slot is selected.
  reset(g); put(g, 0, kGas, 1); put(g, 4, kPipsi, 1); A::eq(g) = 4;
  A::use(g);
  check(A::inv(g).slot(0).id == kGas && A::inv(g).slot(0).count == 1 && A::inv(g).slot(4).empty(),
        "USE Pipsi in slot 4: gasoline in slot 0 untouched");
  // 5. Inventory Cross uses the FOCUSED bag slot and restores the hand slot.
  reset(g); put(g, 0, kApple, 1); put(g, 9, kPipsi, 1); A::eq(g) = 1;
  A::use_bag(g, 9);
  check(A::inv(g).slot(9).empty() && A::inv(g).slot(0).count == 1 && A::eq(g) == 1,
        "use_bag_slot(9) uses slot 9, keeps equipped=1");
  // 6. Drop focused / equipped slot.
  reset(g); put(g, 0, kApple, 1); put(g, 3, kPipsi, 2);
  A::drop(g, 3);
  check(A::inv(g).slot(3).count == 1 && A::inv(g).slot(0).count == 1, "drop slot 3 drops from slot 3 only");
  A::eq(g) = 3; A::drop_eq(g);
  check(A::inv(g).slot(3).empty() && A::inv(g).slot(0).count == 1, "drop equipped (3) drops from slot 3 only");
  // 7. Equip: hotbar slot selects; bag slot swaps into the hand slot.
  reset(g); put(g, 1, kKnife, 1); put(g, 10, kPipsi, 1); A::eq(g) = 0;
  A::equip_bag(g, 4);
  check(A::eq(g) == 4, "equip_bag_slot(4) selects hotbar 4");
  A::eq(g) = 2; A::equip_bag(g, 10);
  check(A::inv(g).slot(2).id == kPipsi && A::inv(g).slot(10).empty() && A::inv(g).slot(1).id == kKnife,
        "equip_bag_slot(10) moves focused stack into hand slot 2");
  // 8. Reload loads ammo for the SELECTED weapon.
  {
    ItemId gun = 0; const WeaponDef* w = 0;
    for (ItemId id = 1; id < 700 && !gun; ++id) {
      const WeaponDef* c = find_weapon(id);
      if (c && !c->stats.is_melee && c->mag_capacity > 1 && c->ammo_id > 0) { gun = id; w = c; }
    }
    if (!gun) check(false, "found a magazine weapon for reload test");
    else {
      reset(g); put(g, 0, kKnife, 1); put(g, 3, gun, 1); put(g, 6, w->ammo_id, w->mag_capacity + 5);
      A::eq(g) = 3; A::reload(g);
      check(A::mag(g) == w->mag_capacity && A::inv(g).slot(6).count == 5, "reload fills selected gun from its ammo");
      reset(g); put(g, 0, gun, 1); put(g, 6, w->ammo_id, 10); A::eq(g) = 1; // knife-less empty hand
      A::reload(g);
      check(A::mag(g) == 0 && A::inv(g).slot(6).count == 10, "reload with empty selected slot loads nothing");
    }
  }
  // 9. Selection stays valid after the selected stack is consumed.
  reset(g); put(g, 5, kPipsi, 1); A::eq(g) = 5;
  A::use(g);
  check(A::eq(g) == 5 && A::inv(g).slot(5).empty(), "equipped index kept (slot now empty) after consume");
  A::use(g);
  check(A::eq(g) == 5, "USE on now-empty selection is a no-op");
  // 10. Inventory focus clamp + open sync.
  reset(g); A::eq(g) = 6;
  A::ui(g).opened = true; A::ui(g).focus_bag_slot = 999;
  InputState none = InputState();
  A::inv_ui(g, none);
  check(A::ui(g).focus_bag_slot == 6, "out-of-range inventory focus clamps to equipped slot");
  // 11. Craft: selected recipe is the one crafted; index clamps.
  {
    const int n = craft_recipe_count();
    int k = -1;
    for (int i = n - 1; i > 0; --i) {
      const CraftRecipe* r = craft_recipe_at(i);
      if (r && !r->requires_fire && r->result_id != r->ing_a && r->result_id != r->ing_b) { k = i; break; }
    }
    if (k < 0) check(false, "found a non-fire recipe");
    else {
      const CraftRecipe* r = craft_recipe_at(k);
      reset(g); put(g, 8, r->ing_a, r->count_a);
      if (r->ing_b > 0) put(g, 9, r->ing_b, r->count_b);
      if (r->tool_id > 0) put(g, 10, r->tool_id, 1);
      if (r->tool_any_knife) put(g, 11, 42, 1);
      A::craft_idx(g) = k; A::craft(g);
      bool has = false;
      for (int i = 0; i < A::inv(g).slot_count(); ++i)
        if (A::inv(g).slot(i).id == r->result_id && A::inv(g).slot(i).count > 0) has = true;
      check(has, "craft makes the SELECTED recipe's result");
    }
    A::craft_idx(g) = n + 7;
    A::craft_ui(g, none);
    check(A::craft_idx(g) >= 0 && A::craft_idx(g) < n, "craft index clamped into range");
  }
  // 12. Quick-slot switching.
  check(Game::next_hotbar_slot(2, 5, 0) == 4, "digit 5 -> slot 4");
  check(Game::next_hotbar_slot(7, 0, 1) == 0, "delta +1 wraps 7 -> 0");
  check(Game::next_hotbar_slot(0, 0, -1) == 7, "delta -1 wraps 0 -> 7");
  check(Game::next_hotbar_slot(42, 0, 0) == 0, "bad index clamps to 0");
  check(Game::next_hotbar_slot(3, 9, 0) == 3, "digit 9 ignored (8 quick slots)");

  delete gp;
  std::printf("test_selection: %d failure(s)\n", g_fail);
  return g_fail ? 1 : 0;
}
