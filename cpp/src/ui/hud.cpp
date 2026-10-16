#include "mdaz/hud.hpp"
#include "mdaz/game.hpp"
#include "mdaz/map_labels.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/types.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"
#include "mdaz/zombie.hpp"

#include <cmath>
#include <cstdio>

namespace mdaz {

namespace {

void draw_bar(IPlatform& p, float x, float y, float w, float h, float fill01,
              float r, float g, float b) {
  if (fill01 < 0.f) fill01 = 0.f;
  if (fill01 > 1.f) fill01 = 1.f;
  p.draw_rect(x, y, w, h, 0.1f, 0.1f, 0.1f, 0.85f);
  p.draw_rect(x, y, w * fill01, h, r, g, b, 1.f);
}

} // namespace

void draw_debug_hud(IPlatform& platform, const Game& game) {
  const Player& pl = game.player();
  char buf[160];

  draw_bar(platform, 8, 8, 120, 8, pl.survival.blood / 100.f, 0.75f, 0.15f, 0.12f);
  draw_bar(platform, 8, 18, 120, 6, pl.survival.food / 100.f, 0.75f, 0.55f, 0.15f);
  draw_bar(platform, 8, 26, 120, 6, pl.survival.water / 100.f, 0.2f, 0.45f, 0.85f);
  draw_bar(platform, 8, 34, 120, 6, pl.survival.temperature / 100.f, 0.85f, 0.35f, 0.1f);

  std::snprintf(buf, sizeof(buf), "HP%.0f F%.0f W%.0f T%.0f",
                pl.survival.blood, pl.survival.food, pl.survival.water, pl.survival.temperature);
  platform.draw_text(134, 8, buf, 0.95f, 0.95f, 0.9f);

  if (pl.survival.bleeding > 0.f) {
    platform.draw_text(134, 20, "BLEEDING - ZEDS", 1.f, 0.25f, 0.2f);
  }
  if (pl.survival.infected) {
    platform.draw_text(220, 20, "INFECTED FEVER", 0.45f, 0.95f, 0.35f);
  }
  if (pl.survival.food < 20.f) {
    platform.draw_text(8, 84, pl.survival.food < 8.f ? "STARVING NO SPRINT" : "STARVING",
                       0.95f, 0.55f, 0.25f);
  }
  if (pl.survival.water < 20.f) {
    platform.draw_text(90, 84, pl.survival.water < 8.f ? "THIRSTY NO SPRINT" : "THIRSTY",
                       0.45f, 0.7f, 0.95f);
  }
  if (pl.survival.temperature < 25.f) {
    platform.draw_text(160, 84, "FREEZING", 0.55f, 0.75f, 0.95f);
  }
  if (game.indoor_t() > 0.f) {
    platform.draw_text(340, 58, "INSIDE", 0.7f, 0.65f, 0.45f);
  }
  if (game.tent_warmth_t() > 0.f) {
    platform.draw_text(340, 70, "TENT", 0.65f, 0.8f, 0.45f);
  }
  if (pl.torch_on) {
    char tbuf[32];
    std::snprintf(tbuf, sizeof(tbuf), "TORCH %.0f", game.torch_battery());
    platform.draw_text(270, 20, tbuf, 1.f, 0.85f, 0.3f);
  } else if (game.torch_battery() <= 1.f) {
    platform.draw_text(270, 20, "NO BATT", 0.5f, 0.4f, 0.3f);
  }
  if (pl.nvg_on) {
    char nbuf[32];
    std::snprintf(nbuf, sizeof(nbuf), "NVG %.0f", game.torch_battery());
    platform.draw_text(320, 32, nbuf, 0.35f, 0.95f, 0.4f);
  }
  if (game.inv_full_flash() > 0.f) {
    platform.draw_text(180, 140, "INV FULL", 1.f, 0.55f, 0.2f);
  }
  {
    char pbuf[128];
    pbuf[0] = 0;
    int n = 0;
    if (pl.perk_metabolism) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "MET ");
    if (pl.perk_hamster) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "HAM ");
    if (pl.perk_camel) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "CAM ");
    if (pl.perk_runner) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "RUN ");
    if (pl.perk_regen) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "RGN ");
    if (pl.perk_snowborn) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "SNW ");
    if (pl.perk_hunter) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "HNT ");
    else if (pl.animals_killed > 0)
      n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "HNT%d/5 ", pl.animals_killed);
    if (pl.perk_farmer) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "FRM ");
    else if (pl.forage_count > 0)
      n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "FRM%d/8 ", pl.forage_count);
    if (n > 0) platform.draw_text(8, 70, pbuf, 0.55f, 0.7f, 0.85f);
  }
  if (game.zombies().scream_active()) {
    platform.draw_text(180, 100, "SCREAM", 1.f, 0.3f, 0.2f);
  }
  {
    // Buried ambush warning when a buried zed is close underground
    for (const Zombie* z = game.zombies().begin(); z != game.zombies().end(); ++z) {
      if (!z->alive || z->type != Zed_Buried) continue;
      const float dx = z->pos.x - pl.pos.x;
      const float dy = z->pos.y - pl.pos.y;
      if (dx * dx + dy * dy < 100.f * 100.f && z->ambush_boost <= 0.f) {
        platform.draw_text(180, 112, "UNEASY...", 0.6f, 0.5f, 0.35f);
        break;
      }
    }
  }
  if (!game.in_bunker() && game.weather().rain > 0.75f && game.indoor_t() <= 0.f &&
      game.tent_warmth_t() <= 0.f) {
    platform.draw_text(300, 96, "MATCHES WET", 0.7f, 0.65f, 0.4f);
  }
  if (game.wetness() > 40.f) {
    char wbuf[24];
    std::snprintf(wbuf, sizeof(wbuf), "WET %.0f", game.wetness());
    platform.draw_text(340, 46, wbuf, 0.45f, 0.65f, 0.9f);
  }
  if (game.wearing_gasmask()) {
    char fbuf[28];
    std::snprintf(fbuf, sizeof(fbuf), "FILTER %.0f", game.gasmask_filter());
    const float fr = game.gasmask_filter() < 15.f ? 0.95f : 0.55f;
    platform.draw_text(340, 58, fbuf, fr, 0.7f, 0.45f);
  }
  if (game.mag_capacity_equipped() > 0 && game.mag_rounds() <= 0 &&
      game.ammo_for_equipped() > 0) {
    platform.draw_text(200, 132, "V RELOAD", 0.9f, 0.8f, 0.35f);
  }
  // Near-fire hint is drawn via temperature color already; bag tip covers T=fire kit
  if (game.crouching()) {
    platform.draw_text(320, 20, "CROUCH AIM", 0.55f, 0.7f, 0.55f);
  } else if (game.driving()) {
    platform.draw_text(320, 20, "DRIVING", 0.6f, 0.75f, 1.f);
    const Vehicle* v = game.driven_vehicle();
    if (v) {
      char fbuf[48];
      std::snprintf(fbuf, sizeof(fbuf), "FUEL %.0f HP %.0f", v->fuel, v->hp);
      const float fr = v->fuel < 20.f ? 0.95f : 0.6f;
      const float fg = v->fuel < 20.f ? 0.35f : 0.75f;
      platform.draw_text(240, 228, fbuf, fr, fg, 1.f);
      if (v->fuel < 20.f && v->type != Vehicle_Bike)
        platform.draw_text(240, 216, "LOW FUEL", 0.95f, 0.4f, 0.2f);
      else if (v->hp < 30.f)
        platform.draw_text(240, 216, "VEHICLE DAMAGED", 0.95f, 0.5f, 0.2f);
      else if (game.weather().is_night &&
               (v->type == Vehicle_Car || v->type == Vehicle_Bike))
        platform.draw_text(240, 216, "HEADLIGHTS", 0.85f, 0.8f, 0.4f);
      if (v->type == Vehicle_Boat) {
        const float m = 420.f;
        const bool shore =
          pl.pos.x < m || pl.pos.y < m ||
          pl.pos.x > kMapLayoutW - m || pl.pos.y > kMapLayoutH - m;
        if (!shore) platform.draw_text(240, 204, "BOAT BEACHED", 0.7f, 0.55f, 0.35f);
      }
    }
  }
  if (game.near_campfire()) {
    bool has_pan = false;
    for (int i = 0; i < game.inventory().slot_count(); ++i) {
      if (game.inventory().slot(i).id == 210) { has_pan = true; break; }
    }
    if (has_pan)
      platform.draw_text(240, 32, "FIRE+PAN - R COOK", 1.f, 0.6f, 0.25f);
    else if (game.weather().is_night)
      platform.draw_text(260, 32, "FIRE - COOK / SLEEP", 1.f, 0.55f, 0.2f);
    else
      platform.draw_text(280, 32, "FIRE - R COOK / REST", 1.f, 0.55f, 0.2f);
  }
  if (game.limp_t() > 0.f) {
    platform.draw_text(160, 148, "R SPLINT: sticks+rags", 0.75f, 0.55f, 0.4f);
  }
  {
    const ItemId eq = game.inventory().slot(game.equipped_slot()).id;
    if (eq == 201)
      platform.draw_text(200, 160, "E DIG (shovel)", 0.55f, 0.7f, 0.45f);
    else if (game.near_choppable_tree() &&
             (eq == 202 || eq == 204 || eq == 206))
      platform.draw_text(200, 160, "E CHOP TREE", 0.55f, 0.8f, 0.4f);
  }
  {
    // Fishing tip near water with rod
    bool has_rod = false;
    for (int i = 0; i < game.inventory().slot_count(); ++i) {
      const ItemId id = game.inventory().slot(i).id;
      if (id == 86 || id == 87) { has_rod = true; break; }
    }
    if (has_rod && !game.in_bunker()) {
      const float m = 400.f;
      const bool near_water =
        pl.pos.x < m || pl.pos.y < m ||
        pl.pos.x > kMapLayoutW - m || pl.pos.y > kMapLayoutH - m;
      if (near_water) {
        bool has_bait = false;
        for (int i = 0; i < game.inventory().slot_count(); ++i) {
          const ItemId bid = game.inventory().slot(i).id;
          if (bid == 6 || bid == 27 || bid == 73 || bid == 74 || bid == 60 || bid == 71) {
            if (game.inventory().slot(i).count > 0) { has_bait = true; break; }
          }
        }
        platform.draw_text(160, 184, has_bait ? "E FISH (+BAIT)" : "E FISH (bait helps)",
                           0.5f, 0.7f, 0.85f);
      }
    }
  }
  {
    const ItemId eq = game.inventory().slot(game.equipped_slot()).id;
    if (eq == 68 || eq == 47 || eq == 48)
      platform.draw_text(200, 160, "E PLANT SEEDS", 0.55f, 0.85f, 0.45f);
    else if (eq == 50)
      platform.draw_text(180, 160, "C FENCE (pile+2 sticks)", 0.65f, 0.55f, 0.35f);
    else if (eq == 53)
      platform.draw_text(180, 160, "C GAS->FIRE or puddle", 0.85f, 0.55f, 0.25f);
    else if (eq == 99)
      platform.draw_text(200, 160, "C PLACE CLAYMORE", 0.85f, 0.4f, 0.25f);
    else if (eq == 56)
      platform.draw_text(200, 160, "C PLACE LANDMINE", 0.9f, 0.35f, 0.2f);
    else if (eq == 83)
      platform.draw_text(200, 160, "C WEAR BANDOLIER", 0.7f, 0.65f, 0.4f);
    else if (eq == 82)
      platform.draw_text(180, 160, "C CUT WIRE/FENCE", 0.7f, 0.6f, 0.45f);
    else if (eq == 205)
      platform.draw_text(180, 160, "E PRY (crowbar)", 0.65f, 0.7f, 0.5f);
  }
  if (game.near_vehicle() && !game.driving()) {
    platform.draw_text(180, 172, "E MOUNT  C+GAS/TOOLBOX", 0.55f, 0.7f, 0.85f);
  }
  if (game.airdrop_alert() > 0.f) {
    platform.draw_text(160, 70, "AIRDROP INBOUND", 0.95f, 0.85f, 0.2f);
  }
  // Flare lure warning uses same area as airdrop — shown via FIRE glow already
  if (game.radio_t() > 0.f) {
    char rbuf[40];
    std::snprintf(rbuf, sizeof(rbuf), "RADIO AIR ETA %.0fs", game.airdrop_eta());
    platform.draw_text(140, 82, rbuf, 0.7f, 0.85f, 0.55f);
  }
  {
    bool has_sil = false;
    float best_scope = 1.f;
    const char* scope_lbl = 0;
    for (int i = 0; i < game.inventory().slot_count(); ++i) {
      const ItemId sid = game.inventory().slot(i).id;
      if (sid == 80 || sid == 81) has_sil = true;
      if (sid == 98 && best_scope > 0.28f) { best_scope = 0.28f; scope_lbl = "LR SCOPE"; }
      else if (sid == 79 && best_scope > 0.32f) { best_scope = 0.32f; scope_lbl = "ACOG"; }
      else if (sid == 78 && best_scope > 0.36f) { best_scope = 0.36f; scope_lbl = "PSO-1"; }
      else if (sid == 77 && best_scope > 0.40f) { best_scope = 0.40f; scope_lbl = "PU SCOPE"; }
      else if (sid == 76 && best_scope > 0.45f) { best_scope = 0.45f; scope_lbl = "RDS"; }
    }
    if (scope_lbl) platform.draw_text(390, 70, scope_lbl, 0.55f, 0.8f, 0.95f);
    if (has_sil) platform.draw_text(390, 82, "SILENCED", 0.45f, 0.7f, 0.55f);
  }
  if (game.gun_jammed()) {
    platform.draw_text(200, 120, "JAMMED - V", 1.f, 0.45f, 0.25f);
  } else if (game.reload_cd() > 0.f) {
    platform.draw_text(200, 120, "RELOAD", 0.85f, 0.75f, 0.35f);
  } else if (game.mag_capacity_equipped() > 0 && game.mag_rounds() <= 0) {
    platform.draw_text(200, 120, "EMPTY - V", 0.9f, 0.55f, 0.3f);
  }
  if (game.adrenaline_t() > 0.f) {
    platform.draw_text(200, 88, "ADRENALINE", 0.95f, 0.55f, 0.3f);
  }
  if (game.limp_t() > 0.f) {
    platform.draw_text(200, 100, "LIMP", 0.9f, 0.5f, 0.35f);
  }
  {
    const int ammo = game.ammo_for_equipped();
    const int cap = game.mag_capacity_equipped();
    if (ammo >= 0) {
      char abuf[40];
      if (cap > 0)
        std::snprintf(abuf, sizeof(abuf), "MAG %d/%d  +%d", game.mag_rounds(), cap, ammo);
      else
        std::snprintf(abuf, sizeof(abuf), "AMMO %d", ammo);
      platform.draw_text(200, 240, abuf, 0.85f, 0.85f, 0.55f);
    }
  }
  if (game.inventory().worn_count() > 0) {
    char wbuf[40];
    std::snprintf(wbuf, sizeof(wbuf), "GEAR %d BAG %d", game.inventory().worn_count(),
                  game.inventory().capacity());
    platform.draw_text(360, 84, wbuf, 0.65f, 0.7f, 0.8f);
  } else {
    char wbuf[24];
    std::snprintf(wbuf, sizeof(wbuf), "BAG %d", game.inventory().capacity());
    platform.draw_text(400, 84, wbuf, 0.55f, 0.6f, 0.65f);
  }
  if (game.quest().active != Quest_None) {
    char qbuf[80];
    std::snprintf(qbuf, sizeof(qbuf), "OBJ %s %d/%d", game.quest().label(),
                  game.quest().progress, game.quest().target);
    platform.draw_text(8, 58, qbuf, 0.85f, 0.8f, 0.45f);
  }

  {
    const float sw = kMapLayoutW / static_cast<float>(kMapSectorsX);
    const float sh = kMapLayoutH / static_cast<float>(kMapSectorsY);
    int sx = static_cast<int>(pl.pos.x / sw);
    int sy = static_cast<int>(pl.pos.y / sh);
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx >= kMapSectorsX) sx = kMapSectorsX - 1;
    if (sy >= kMapSectorsY) sy = kMapSectorsY - 1;
    std::snprintf(buf, sizeof(buf), "D%d PH%.2f Z%d K%d [%d,%d]",
                  pl.days_survived, game.weather().day_phase,
                  game.zombies().alive_count(), pl.zeds_killed, sx, sy);
  }
  platform.draw_text(8, 46, buf, 0.75f, 0.8f, 0.7f);

  if (game.in_bunker()) {
    platform.draw_text(300, 8, "BUNKER WARM", 0.85f, 0.7f, 0.5f);
  } else {
    // Nearest useful POI compass
    float best = 1e12f;
    LocationType best_t = Loc_BunkerEntrance;
    bool found = false;
    for (int i = 0; i < game.mapgen().location_count(); ++i) {
      const MapLocation& L = game.mapgen().location(i);
      if (L.type != Loc_BunkerEntrance && L.type != Loc_Hospital &&
          L.type != Loc_Military && L.type != Loc_GasStation &&
          L.type != Loc_Police && L.type != Loc_Church &&
          L.type != Loc_Firestation) continue;
      const float dx = L.pos.x - pl.pos.x;
      const float dy = L.pos.y - pl.pos.y;
      const float d2 = dx * dx + dy * dy;
      if (d2 < best) { best = d2; best_t = L.type; found = true; }
    }
    if (found) {
      // Bearing letter toward nearest POI (N/NE/E/…)
      float bdx = 0.f, bdy = 0.f;
      for (int i = 0; i < game.mapgen().location_count(); ++i) {
        const MapLocation& L = game.mapgen().location(i);
        if (L.type != best_t) continue;
        const float dx = L.pos.x - pl.pos.x;
        const float dy = L.pos.y - pl.pos.y;
        if (dx * dx + dy * dy <= best + 0.1f) { bdx = dx; bdy = dy; break; }
      }
      const char* dir = "?";
      const float ax = std::fabs(bdx), ay = std::fabs(bdy);
      if (ax < 1.f && ay < 1.f) dir = "HERE";
      else if (ay >= ax * 2.f) dir = (bdy < 0.f) ? "N" : "S";
      else if (ax >= ay * 2.f) dir = (bdx > 0.f) ? "E" : "W";
      else if (bdy < 0.f) dir = (bdx > 0.f) ? "NE" : "NW";
      else dir = (bdx > 0.f) ? "SE" : "SW";
      std::snprintf(buf, sizeof(buf), "%s %s %.0f", location_label(best_t), dir, std::sqrt(best));
      platform.draw_text(260, 8, buf, 0.7f, 0.75f, 0.55f);
    }
  }

  // Hotbar slots 1..8 (medical tinted red, food/drink warm, weapons cool)
  for (int i = 0; i < 8; ++i) {
    const float sx = 8.f + i * 28.f;
    const float sy = 250.f;
    const bool eq = (i == game.equipped_slot());
    float br = eq ? 0.35f : 0.12f, bg = eq ? 0.35f : 0.12f, bb = eq ? 0.2f : 0.12f;
    const InvSlot& sl = game.inventory().slot(i);
    if (!sl.empty()) {
      const ItemDef* d = game.items().find(sl.id);
      if (d) {
        if (d->category == ItemCat_Medical) { br = eq ? 0.55f : 0.28f; bg = 0.12f; bb = 0.12f; }
        else if (d->category == ItemCat_Food || d->category == ItemCat_Drink) {
          br = eq ? 0.4f : 0.22f; bg = eq ? 0.32f : 0.18f; bb = 0.1f;
        } else if (d->category == ItemCat_Weapon || d->category == ItemCat_Melee) {
          br = 0.15f; bg = eq ? 0.3f : 0.18f; bb = eq ? 0.4f : 0.22f;
        }
      }
    }
    platform.draw_rect(sx, sy, 26, 18, br, bg, bb, 0.9f);
    if (!sl.empty()) {
      std::snprintf(buf, sizeof(buf), "%d", sl.id);
      platform.draw_text(sx + 2, sy + 4, buf, 0.9f, 0.9f, 0.85f);
    } else {
      std::snprintf(buf, sizeof(buf), "%d", i + 1);
      platform.draw_text(sx + 8, sy + 4, buf, 0.4f, 0.4f, 0.4f);
    }
  }

  const InvSlot& eq = game.inventory().slot(game.equipped_slot());
  if (!eq.empty()) {
    const WeaponDef* w = find_weapon(eq.id);
    const ItemDef* idef = game.items().find(eq.id);
    if (w) {
      std::snprintf(buf, sizeof(buf), "EQ %s", w->name);
    } else if (idef) {
      std::snprintf(buf, sizeof(buf), "EQ %s", idef->name.c_str());
    } else {
      std::snprintf(buf, sizeof(buf), "EQ #%d", eq.id);
    }
    platform.draw_text(240, 252, buf, 0.85f, 0.9f, 0.7f);
  } else {
    platform.draw_text(240, 252, "EQ FISTS", 0.7f, 0.7f, 0.65f);
  }

  if (game.show_bag()) {
    platform.draw_rect(300, 90, 170, 140, 0.05f, 0.05f, 0.05f, 0.85f);
    platform.draw_text(308, 96, "INVENTORY", 0.9f, 0.85f, 0.7f);
    int row = 0;
    for (int i = 0; i < game.inventory().slot_count() && row < 10; ++i) {
      const InvSlot& sl = game.inventory().slot(i);
      if (sl.empty()) continue;
      const ItemDef* d = game.items().find(sl.id);
      char line[64];
      if (d) std::snprintf(line, sizeof(line), "%dx %s", sl.count, d->name.c_str());
      else std::snprintf(line, sizeof(line), "%dx #%d", sl.count, sl.id);
      platform.draw_text(308, 112.f + row * 12.f, line, 0.8f, 0.8f, 0.75f);
      ++row;
    }
  }

  // Minimap (top-right): sector fog + POIs relative
  {
    const float mx = 400.f, my = 8.f, mw = 72.f, mh = 72.f;
    platform.draw_rect(mx, my, mw, mh, 0.05f, 0.06f, 0.05f, 0.75f);
    const float cell_w = mw / static_cast<float>(kMapSectorsX);
    const float cell_h = mh / static_cast<float>(kMapSectorsY);
    for (int sy = 0; sy < kMapSectorsY; ++sy) {
      for (int sx = 0; sx < kMapSectorsX; ++sx) {
        if (game.sector_visited(sx, sy)) {
          platform.draw_rect(mx + sx * cell_w, my + sy * cell_h, cell_w, cell_h,
                             0.12f, 0.18f, 0.12f, 0.55f);
        } else {
          platform.draw_rect(mx + sx * cell_w, my + sy * cell_h, cell_w, cell_h,
                             0.02f, 0.02f, 0.03f, 0.85f);
        }
      }
    }
    const float scale = 0.004f;
    for (int i = 0; i < game.mapgen().location_count(); ++i) {
      const MapLocation& L = game.mapgen().location(i);
      if (!game.sector_visited(L.sector_x, L.sector_y) && L.type != Loc_BunkerEntrance) continue;
      float px = mx + mw * 0.5f + (L.pos.x - pl.pos.x) * scale;
      float py = my + mh * 0.5f + (L.pos.y - pl.pos.y) * scale;
      if (px < mx || py < my || px > mx + mw - 2 || py > my + mh - 2) continue;
      float r = 0.5f, g = 0.5f, b = 0.3f;
      if (L.type == Loc_BunkerEntrance) { r = 0.95f; g = 0.65f; b = 0.2f; }
      else if (L.type == Loc_Military) { r = 0.25f; g = 0.7f; b = 0.3f; }
      else if (L.type == Loc_Hospital) { r = 0.9f; g = 0.35f; b = 0.35f; }
      else if (L.type == Loc_GasStation) { r = 0.85f; g = 0.75f; b = 0.2f; }
      else if (L.type == Loc_City) { r = 0.75f; g = 0.55f; b = 0.25f; }
      else if (L.type == Loc_Village) { r = 0.55f; g = 0.65f; b = 0.35f; }
      else if (L.type == Loc_Helicrash) { r = 0.9f; g = 0.4f; b = 0.15f; }
      else if (L.type == Loc_Firestation) { r = 0.85f; g = 0.3f; b = 0.2f; }
      else if (L.type == Loc_Police) { r = 0.35f; g = 0.45f; b = 0.95f; }
      else if (L.type == Loc_Church) { r = 0.8f; g = 0.75f; b = 0.55f; }
      else if (L.type == Loc_Secret) { r = 0.85f; g = 0.35f; b = 0.95f; }
      platform.draw_rect(px, py, 2, 2, r, g, b, 1.f);
    }
    if (game.airdrop_alert() > 0.f) {
      const Vec2 ap = game.last_airdrop_pos();
      float px = mx + mw * 0.5f + (ap.x - pl.pos.x) * scale;
      float py = my + mh * 0.5f + (ap.y - pl.pos.y) * scale;
      if (px >= mx && py >= my && px <= mx + mw - 2 && py <= my + mh - 2) {
        platform.draw_rect(px, py, 3, 3, 1.f, 0.5f, 0.1f, 1.f);
      }
    }
    // Player sector marker on fog grid + local relative blip
    {
      const float sw = kMapLayoutW / static_cast<float>(kMapSectorsX);
      const float sh = kMapLayoutH / static_cast<float>(kMapSectorsY);
      int psx = static_cast<int>(pl.pos.x / sw);
      int psy = static_cast<int>(pl.pos.y / sh);
      if (psx < 0) psx = 0;
      if (psy < 0) psy = 0;
      if (psx >= kMapSectorsX) psx = kMapSectorsX - 1;
      if (psy >= kMapSectorsY) psy = kMapSectorsY - 1;
      platform.draw_rect(mx + psx * cell_w, my + psy * cell_h, cell_w, cell_h,
                         0.25f, 0.55f, 0.3f, 0.45f);
    }
    platform.draw_rect(mx + mw * 0.5f - 1.f, my + mh * 0.5f - 1.f, 3, 3, 0.3f, 0.9f, 0.4f, 1.f);
  }

  platform.draw_text(8, 238, "Z atk F grenade/stick G drop V reload R craft", 0.5f, 0.55f, 0.5f);
}

} // namespace mdaz
