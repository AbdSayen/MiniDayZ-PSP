// Item 5: chopping trees 1:1 with data.js.
//  * gui_btn_interact frame 7 (loot_checker2 sid5546315147952857): player IV3 == 0, IV4 == 1
//    (Split Axe, item 202) or 5 (Fireaxe, item 204), doorchecker LOS (range 70) to an obst_base
//    with frame 10 (tree trunk) within 100 px.
//  * Interact (sid7921730188078593): nearest obst_base (PickDistance) <= 100 -> walk_marker IV0 = 8
//    at the obst_base moved 10 px toward the player (at the player when <= 30 px); AltMove 100.
//  * walk_marker reached (sid4081213929954104): Reloading_time_melee == 0 -> spawn
//    melee_swing_general at the player, angle = player angle, Punch_Swipes_0/1 [zmb];
//    IV4 == 1 -> swing IV0 45, IV4 == 5 -> 60, IV7 (chop) = true; Reloading_time_melee for
//    t909 IV4 (Split Axe 1.5 s, Fireaxe 1.6 s).
//  * Melee_2_auto (attack): IV4 == 1 / 5 also sets IV7 on its swing (sid6326427994993125).
//  * melee_swing_general: Bullet speed 100 (family New_bullet), Fade wait 0.1 + out 0.1 -> destroy;
//    collision poly = 10 x 1 px strip ahead of the origin.
//  * t910 OnCollision obst_base (sid5202978245239527), IV7: obst frame 10 overlapping a tree with
//    IV0 > 0 -> IV0 -= 1, chop/chop2/3/4 at the swing; IV0 <= 0 -> treefall_02/03 (-5 dB, not
//    positional), tree frame 1, Spawn_drop list per tree type at tree.x + random(-30,30),
//    tree.y + 40 + random(10), obst_base frame 8, tutorTreeChopped = 1, ach_lumberjack IV2 += 1.
//    The swing IV2 (0) <= choose(1,2) -> test_bullet_hit + Destroy.
#include "mdaz/game.hpp"
#include "mdaz/balance_globals.hpp"
#include "mdaz/c2_sfx.hpp"
#include "mdaz/platform.hpp"
#include <cmath>
#include <cstring>

namespace mdaz {

namespace {
// Tree types per create_map_element cat 1 sub (1..6). Frame 0: size, hotspot, Imagepoint 1
// (trunk; trees_[] stores it). Frame 1 (stump): sheet, rect, hotspot. data.js type animations.
struct TreeGeom {
  float w0, h0, hx0, hy0, ip0x, ip0y;
  const char* sheet1; int fx1, fy1, fw1, fh1; float hx1, hy1;
  int drops[4]; int ndrops;
};
const TreeGeom kTreeGeom[6] = {
  {47, 92, 0.5106f, 0.5f, 0.5319f, 0.9239f, "tree_pine-sheet0", 1, 95, 78, 32, 0.6923f, -0.3750f, {50, 50}, 2},              // t175
  {53, 97, 0.5094f, 0.5052f, 0.4151f, 0.9175f, "tree_leaves-sheet0", 56, 1, 71, 52, 0.6761f, 0.0385f, {38, 38, 50}, 3},       // t198
  {57, 94, 0.4561f, 0.5106f, 0.4561f, 0.9149f, "tree_leaves_2-sheet1", 0, 0, 85, 44, 0.6706f, -0.0455f, {38, 38, 50, 37}, 4}, // t200
  {50, 119, 0.52f, 0.5126f, 0.54f, 0.9496f, "tree_pine2-sheet1", 0, 0, 110, 44, 0.8f, -0.3864f, {50, 50}, 2},                 // t569
  {53, 117, 0.5094f, 0.5043f, 0.4151f, 0.9316f, "tree_leaves_new2-sheet1", 0, 0, 87, 52, 0.8276f, -0.1538f, {50, 50, 38}, 3}, // t570
  {64, 115, 0.5156f, 0.513f, 0.5f, 0.9217f, "tree_leaves_new3-sheet1", 0, 0, 107, 54, 0.7757f, -0.1667f, {50, 50, 38, 38}, 4},// t574
};
// obst_base frame 10 (15x17) and frame 8 (14x16) collision triangles, relative to the origin.
const float kTri10[6] = {0.f * 15.f, -0.5882f * 17.f, 0.5333f * 15.f, 0.2353f * 17.f, -0.4667f * 15.f, 0.2353f * 17.f};
const float kTri8[6] = {0.0089f * 14.f, -0.5547f * 16.f, 0.3214f * 14.f, 0.2656f * 16.f, -0.4554f * 14.f, 0.2031f * 16.f};

bool in_tri(const float* t, float px, float py) {
  float sign = 0.f;
  for (int e = 0; e < 3; ++e) {
    const float x0 = t[e * 2], y0 = t[e * 2 + 1];
    const float x1 = t[((e + 1) % 3) * 2], y1 = t[((e + 1) % 3) * 2 + 1];
    const float c = (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);
    if (c != 0.f) {
      if (sign == 0.f) sign = c;
      else if ((c > 0.f) != (sign > 0.f)) return false;
    }
  }
  return true;
}
inline float dist2(Vec2 a, Vec2 b) { const float x = a.x - b.x, y = a.y - b.y; return x * x + y * y; }
float melee_reload_iv4(ItemId id) { return c2_melee_reload(id); }
} // namespace

// Pick-up events (Items_on_ground -> player_collision_base IV4) and Drop_melee (knives 7/8/9 ->
// items 44/42/43): Split Axe 1, shovel 2, Pipe Wrench 3, Baseball Bat 4, Fireaxe 5, Crowbar 6,
// Pickaxe 10, Pitchfork 11, Sledgehammer 12, sword 15, pan 16. 0 = Fist.
int c2_melee_iv4(ItemId id) {
  switch (id) {
    case 202: return 1;  case 201: return 2;  case 200: return 3;  case 203: return 4;
    case 204: return 5;  case 205: return 6;  case 44: return 7;   case 42: return 8;
    case 43: return 9;   case 206: return 10; case 207: return 11; case 208: return 12;
    case 209: return 15; case 210: return 16;
    default: return 0;
  }
}
// t909 IV4 = Reloading_time_melee Wait (layout instance defaults; knives = knife_any t637 0.8).
float c2_melee_reload(ItemId id) {
  switch (id) {
    case 202: return 1.5f; case 201: return 1.2f; case 200: return 1.3f; case 203: return 1.f;
    case 204: return 1.6f; case 205: return 1.2f; case 42: case 43: case 44: return 0.8f;
    case 206: return 1.3f; case 207: return 1.2f; case 208: return 1.8f; case 209: return 1.5f;
    case 210: return 0.9f;
    default: return 0.f;
  }
}
// Melee_2_auto (sid2098275646157213) melee_swing_general IV0 per player_collision_base IV4.
int c2_melee_swing_dmg(int iv4) {
  switch (iv4) {
    case 1: return 50;  case 2: return 25;  case 3: return 30;  case 4: return 35;  case 5: return 60;
    case 6: return 25;  case 7: return 21;  case 8: return 20;  case 9: return 19;  case 10: return 44;
    case 11: return 44; case 12: return 40; case 13: return 48; case 15: return 75; case 16: return 33;
    default: return 10;  // melee_swing_general instance default IV0
  }
}
// Crit chance (round(random(100)) <= N) for perk_meleeatk 0 / 1.
int c2_melee_crit_chance(int iv4, bool perk) {
  int base;
  switch (iv4) {
    case 0: case 9: base = 10; break;
    case 2: case 8: case 10: case 11: case 12: case 13: base = 15; break;
    case 7: base = 20; break;
    case 1: case 3: case 4: case 5: case 6: base = 25; break;
    case 15: base = 35; break;
    case 16: base = 40; break;
    default: return -1;   // IV4 14: no crit event
  }
  return base + (perk ? 10 : 0);
}

// Stump draw (tree frame 1). sx, sy = trunk point (Imagepoint 1 of frame 0) on screen.
void draw_c2_tree_stump(IPlatform* p, int sub, float sx, float sy, float sc) {
  if (!p || sub < 1 || sub > 6) return;
  const TreeGeom& g = kTreeGeom[sub - 1];
  if (!p->has_sprite(g.sheet1)) return;
  // Object position (hotspot) = trunk - (ip0 - hot0) * size0; frame 1 keeps the position.
  const float ox = sx - (g.ip0x - g.hx0) * g.w0 * sc, oy = sy - (g.ip0y - g.hy0) * g.h0 * sc;
  const float w = g.fw1 * sc, h = g.fh1 * sc;
  p->draw_sprite_frame(g.sheet1, ox - g.hx1 * w, oy - g.hy1 * h, w, h, g.fx1, g.fy1, g.fw1, g.fh1, 0.f, 1.f);
}

void Game::reset_chop_state() {
  std::memset(tree_frame_, 0, sizeof(tree_frame_));
  for (int i = 0; i < kMaxSwings; ++i) swings_[i].alive = false;
  chop_walk_ = -1;
  tutor_tree_chopped_ = 0;
}

Vec2 Game::tree_object_pos(int ti) const {
  const int sub = tree_sub_[ti];
  if (sub < 1 || sub > 6) return trees_[ti];
  const TreeGeom& g = kTreeGeom[sub - 1];
  return Vec2(trees_[ti].x - (g.ip0x - g.hx0) * g.w0, trees_[ti].y - (g.ip0y - g.hy0) * g.h0);
}

bool Game::tree_obst_contains(int ti, float px, float py) const {
  return in_tri(tree_frame_[ti] ? kTri8 : kTri10, px - trees_[ti].x, py - trees_[ti].y);
}

int Game::nearest_tree_obst(Vec2 p, float& d2) const {
  int best = -1; d2 = 0.f;
  for (int i = 0; i < tree_count_; ++i) {
    if (tree_sub_[i] < 1 || tree_sub_[i] > 6) continue;   // tree_block uses big_obstacle_base
    const float dx = trees_[i].x - p.x, dy = trees_[i].y - p.y, d = dx * dx + dy * dy;
    if (best < 0 || d < d2) { best = i; d2 = d; }
  }
  return best;
}

void Game::spawn_melee_swing(float ang_deg, bool chop, float dmg, bool crit, ItemId weapon) {
  for (int i = 0; i < kMaxSwings; ++i) {
    if (swings_[i].alive) continue;
    swings_[i].pos = player_.pos;
    swings_[i].ang = ang_deg * 0.01745329252f;
    swings_[i].life = 0.2f;     // Fade: wait 0.1 + fade out 0.1, destroy
    swings_[i].chop = chop;
    swings_[i].dmg = dmg;
    swings_[i].crit = crit;
    swings_[i].weapon = weapon;
    swings_[i].alive = true;
    return;
  }
}

// Hitting_obstacles Else / obstacle branches: hard_ground_1/2 at the bullet, tag ground_hit,
// only when that tag is not already playing.
void Game::swing_hard_ground(Vec2 at) {
  if (audio_ && audio_->is_tag_playing("ground_hit")) return;
  sfx_at((sfx_rng_.next_u32() & 1u) ? "hard_ground_2" : "hard_ground_1", 0.f, at, "ground_hit");
}

void Game::chop_tree_hit(int ti, Vec2 at) {
  tree_hp_[ti] = static_cast<unsigned char>(tree_hp_[ti] - 1);
  sfx_at(c2_choose_chop(sfx_rng_), 0.f, at);
  if (tree_hp_[ti] > 0) return;
  sfx2d(c2_choose_treefall(sfx_rng_), -5.f);
  tree_frame_[ti] = 1;                       // tree frame 1; obst_base frame 8
  const TreeGeom& g = kTreeGeom[tree_sub_[ti] - 1];
  const Vec2 tp = tree_object_pos(ti);
  for (int k = 0; k < g.ndrops; ++k) {
    const float x = tp.x + (-30.f + rng_.next_float() * 60.f);   // random(-30, 30)
    const float y = tp.y + 40.f + rng_.next_float() * 10.f;      // random(10)
    const ItemId id = static_cast<ItemId>(g.drops[k]);
    loot_.spawn_item(Vec2(x, y), id, spawn_drop_count(id));
  }
  tutor_tree_chopped_ = 1;
  if (balance::SEED_RUN == 0) ++ach_lumberjack_;   // Check_ach_unlocks: KNOWN_ISSUES
}

namespace {
struct NpcHitPoly { const char* base; const char* anim; int frame, w, h; float hx, hy; int n; float p[16]; };
const NpcHitPoly kNpcHitPolys[] = {
#include "npc_hit_polys.inc"
};
bool seg_cross(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy) {
  const float d1 = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
  const float d2 = (bx - ax) * (dy - ay) - (by - ay) * (dx - ax);
  const float d3 = (dx - cx) * (ay - cy) - (dy - cy) * (ax - cx);
  const float d4 = (dx - cx) * (by - cy) - (dy - cy) * (bx - cx);
  return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0));
}
bool pt_in_poly(float x, float y, const float* px, const float* py, int n) {
  bool in = false;
  for (int i = 0, j = n - 1; i < n; j = i++)
    if (((py[i] > y) != (py[j] > y)) && (x < (px[j] - px[i]) * (y - py[i]) / (py[j] - py[i]) + px[i])) in = !in;
  return in;
}
bool seg_hits_poly(Vec2 a, Vec2 b, const float* px, const float* py, int n) {
  if (pt_in_poly(a.x, a.y, px, py, n) || pt_in_poly(b.x, b.y, px, py, n)) return true;
  for (int i = 0, j = n - 1; i < n; j = i++)
    if (seg_cross(a.x, a.y, b.x, b.y, px[j], py[j], px[i], py[i])) return true;
  return false;
}
} // namespace

bool seg_hits_box(Vec2 a, Vec2 b, float x0, float y0, float x1, float y1) {
  const float px[4] = {x0, x1, x1, x0}, py[4] = {y0, y0, y1, y1};
  return seg_hits_poly(a, b, px, py, 4);
}

// Collision poly of an animal's current frame, sampled with the same anim/time as the renderer.
bool Game::animal_swing_hit(int ai, Vec2 a, Vec2 b) {
  const Animal* an = animals_.at(ai);
  const Vec2 o = an->pos;
  if (an->type == Animal_Rabbit) return seg_hits_box(a, b, o.x - 9.f, o.y - 9.f, o.x + 9.f, o.y + 9.f);
  if (an->type == Animal_Dog || (an->type != Animal_Wolf && an->type != Animal_Deer))
    return seg_hits_box(a, b, o.x - 15.f, o.y - 15.f, o.x + 15.f, o.y + 15.f);   // dog_eyes 30x30
  const char* base = (an->type == Animal_Wolf) ? "wolf_skin" : "deer_skin";
  const float fdeg = std::atan2(player_.pos.y - o.y, player_.pos.x - o.x) * 57.2957795f;
  const char* prefix = (dist2(o, player_.pos) < 140.f * 140.f) ? "run" : "idle";
  char aname[40];
  CharAnims::make_name(aname, (int)sizeof(aname), prefix, fdeg);
  int fi = 0;
  if (const CharAnim* ca = char_anims_.find(base, aname))
    char_anims_.sample(ca, state_timer_ + static_cast<float>(ai) * 0.23f, &fi);
  const NpcHitPoly* hp = 0;
  for (unsigned k = 0; k < sizeof(kNpcHitPolys) / sizeof(kNpcHitPolys[0]); ++k) {
    const NpcHitPoly& e = kNpcHitPolys[k];
    if (std::strcmp(e.base, base) == 0 && std::strcmp(e.anim, aname) == 0 && e.frame == fi) { hp = &e; break; }
  }
  if (!hp) return false;
  float px[8], py[8]; int n = hp->n;
  if (n == 0) {
    const float l = o.x - hp->hx * hp->w, t = o.y - hp->hy * hp->h;
    px[0] = l; py[0] = t; px[1] = l + hp->w; py[1] = t; px[2] = l + hp->w; py[2] = t + hp->h; px[3] = l; py[3] = t + hp->h; n = 4;
  } else {
    for (int k = 0; k < n; ++k) { px[k] = o.x + hp->p[k * 2] * hp->w; py[k] = o.y + hp->p[k * 2 + 1] * hp->h; }
  }
  return seg_hits_poly(a, b, px, py, n);
}

void Game::update_melee_swings(float dt) {
  for (int i = 0; i < kMaxSwings; ++i) {
    MeleeSwing& s = swings_[i];
    if (!s.alive) continue;
    s.life -= dt;
    if (s.life <= 0.f) { s.alive = false; continue; }
    const float cx = std::cos(s.ang), cy = std::sin(s.ang);
    s.pos.x += cx * 100.f * dt;
    s.pos.y += cy * 100.f * dt;
    // OnCollision with NPCs. The swing poly (frame 10x21, hotspot 0/0.571) is the 10x1 strip
    // ahead of the origin; NPC collision polys from data.js: zed skins 30x30 bbox (tank 40x40),
    // bots 30x30, dog_eyes 30x30, rabbit_skin 18x18, wolf_skin/deer_skin per animation frame.
    {
      const float q = s.dmg * 0.25f;
      bool hit_npc = false;
      const Vec2 s0 = s.pos, s1(s.pos.x + cx * 10.f, s.pos.y + cy * 10.f);
      for (Zombie* z = zombies_.begin(); z != zombies_.end() && !hit_npc; ++z) {
        if (!z->alive) continue;
        const float hw = (z->type == Zed_Tank) ? 20.f : 15.f;
        if (!seg_hits_box(s0, s1, z->pos.x - hw, z->pos.y - hw, z->pos.x + hw, z->pos.y + hw)) continue;
        const float d = std::floor(s.dmg - q + rng_.next_float() * 2.f * q + 0.5f);
        spawn_fx(Fx_Hit, z->pos, 0.f, 0.12f);
        hurt_zombie(*z, d, s.weapon);
        hit_npc = true;
      }
      for (int ai = 0; ai < animals_.count() && !hit_npc; ++ai) {
        Animal* a = animals_.at(ai);
        if (!a->alive || !animal_swing_hit(ai, s0, s1)) continue;
        const float d = std::floor(s.dmg - q + rng_.next_float() * 2.f * q + 0.5f);
        spawn_fx(Fx_Hit, a->pos, 0.f, 0.12f);
        hurt_animal(*a, d);
        hit_npc = true;
      }
      for (Bandit* b = bandits_.begin(); b != bandits_.end() && !hit_npc; ++b) {
        if (!b->alive || !seg_hits_box(s0, s1, b->pos.x - 15.f, b->pos.y - 15.f, b->pos.x + 15.f, b->pos.y + 15.f)) continue;
        const float d = std::floor(s.dmg - q + rng_.next_float() * 2.f * q + 0.5f);
        hurt_bandit(*b, d);
        hit_npc = true;
      }
      // t909 IV6 -= 0.2 / 0.5 / 1 by IV15 (durability): port has no per-item IV6 (KNOWN_ISSUES).
      if (hit_npc) { ++melee_hits_; s.alive = false; continue; }
    }
    if (in_bunker_) continue;
    // OnCollision obst_base: the 10 px strip ahead of the origin against the trunk triangles.
    int hit = -1;
    for (int ti = 0; ti < tree_count_ && hit < 0; ++ti) {
      if (tree_sub_[ti] < 1 || tree_sub_[ti] > 6) continue;
      const float dx = trees_[ti].x - s.pos.x, dy = trees_[ti].y - s.pos.y;
      if (dx * dx + dy * dy > 30.f * 30.f) continue;
      for (int k = 0; k <= 10; ++k) {
        if (tree_obst_contains(ti, s.pos.x + cx * k, s.pos.y + cy * k)) { hit = ti; break; }
      }
    }
    if (hit >= 0) {
      // IV7 (chop) branch: frame 10 obst overlapping the tree with IV0 > 0; Else: hard_ground.
      if (s.chop && tree_frame_[hit] == 0 && tree_hp_[hit] > 0) chop_tree_hit(hit, s.pos);
      else swing_hard_ground(s.pos);
      // t909 IV6 -= 1 (durability) + Items_update: port has no per-item IV6 (KNOWN_ISSUES).
      spawn_fx(Fx_Hit, s.pos, 0.f, 0.2f);      // test_bullet_hit + Destroy
      s.alive = false;
      continue;
    }
    // Hitting_obstacles: buildings 927 / obstacles 202,339,755 -> hard_ground (within
    // Player_Hear_radius: the swing is always within 20 px of the player) + Destroy.
    if (building_blocks(s.pos, Vec2(s.pos.x + cx * 10.f, s.pos.y + cy * 10.f))) {
      swing_hard_ground(s.pos);
      spawn_fx(Fx_Hit, s.pos, 0.f, 0.2f);
      s.alive = false;
    }
  }
}

bool Game::start_chop_walk() {
  float d2 = 0.f;
  const int ti = nearest_tree_obst(player_.pos, d2);
  if (ti < 0 || d2 > 100.f * 100.f) return false;
  const Vec2 o = trees_[ti];
  const float a = std::atan2(player_.pos.y - o.y, player_.pos.x - o.x);   // angle(obst -> player)
  Vec2 m(o.x + std::cos(a) * 10.f, o.y + std::sin(a) * 10.f);
  if (d2 <= 30.f * 30.f) m = player_.pos;
  chop_walk_ = ti;
  chop_walk_tgt_ = m;
  // SetAngle angle(player_base -> walk_marker) runs before MoveAtAngle: the marker is still at
  // the obst_base, so the player faces the trunk.
  player_.facing_deg = std::atan2(o.y - player_.pos.y, o.x - player_.pos.x) * 57.2957795f;
  return true;
}

// Returns true while AltMove drives the player toward the marker.
bool Game::tick_chop_walk(InputState& in) {
  if (chop_walk_ < 0) return false;
  if (in.move_x != 0.f || in.move_y != 0.f) { chop_walk_ = -1; return false; }   // PSP: stick cancels
  const float dx = chop_walk_tgt_.x - player_.pos.x, dy = chop_walk_tgt_.y - player_.pos.y;
  if (std::fabs(dx) <= 3.f && std::fabs(dy) <= 3.f) {   // player_base 2x2 vs walk_marker 4x4
    chop_walk_ = -1;
    const ItemId eq = inventory_.slot(equipped_slot_).id;
    const float iv4 = melee_reload_iv4(eq);
    if (attack_cd_ <= 0.f && iv4 > 0.f) {            // Reloading_time_melee == 0, melee t909
      // walk_marker IV0 = 8 path (sid4081213929954104): IV4 1 -> IV0 45, IV4 5 -> IV0 60, IV7.
      const int pc4 = c2_melee_iv4(eq);
      const bool chop = pc4 == 1 || pc4 == 5;
      spawn_melee_swing(player_.facing_deg, chop, pc4 == 1 ? 45.f : pc4 == 5 ? 60.f : 10.f, false, eq);
      sfx_at(c2_choose_punch(sfx_rng_), 0.f, player_.pos, "zmb");
      melee_anim_t_ = 0.35f;
      attack_cd_ = iv4;
    }
    return false;
  }
  const float l = std::sqrt(dx * dx + dy * dy);
  in.move_x = dx / l; in.move_y = dy / l; in.run = false;
  return true;
}

} // namespace mdaz
