#include "mdaz/hud.hpp"
#include "mdaz/game.hpp"
#include "mdaz/mapgen.hpp"
#include "mdaz/platform.hpp"
#include "mdaz/quest.hpp"
#include "mdaz/vehicle.hpp"
#include "mdaz/weapons.hpp"

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
    platform.draw_text(134, 20, "BLEEDING", 1.f, 0.25f, 0.2f);
  }
  if (pl.survival.infected) {
    platform.draw_text(220, 20, "SICK", 0.55f, 0.9f, 0.35f);
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
    char pbuf[96];
    pbuf[0] = 0;
    int n = 0;
    if (pl.perk_metabolism) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "MET ");
    if (pl.perk_hamster) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "HAM ");
    if (pl.perk_camel) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "CAM ");
    if (pl.perk_runner) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "RUN ");
    if (pl.perk_hunter) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "HNT ");
    if (pl.perk_farmer) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "FRM ");
    if (pl.perk_regen) n += std::snprintf(pbuf + n, sizeof(pbuf) - n, "RGN ");
    if (n > 0) platform.draw_text(8, 70, pbuf, 0.55f, 0.7f, 0.85f);
  }
  if (game.zombies().scream_active()) {
    platform.draw_text(180, 100, "SCREAM", 1.f, 0.3f, 0.2f);
  }
  if (!game.in_bunker() && game.weather().rain > 0.7f) {
    platform.draw_text(340, 46, "WET", 0.45f, 0.65f, 0.9f);
  }
  // Near-fire hint is drawn via temperature color already; bag tip covers T=fire kit
  if (game.driving()) {
    platform.draw_text(320, 20, "DRIVING", 0.6f, 0.75f, 1.f);
    const Vehicle* v = game.driven_vehicle();
    if (v) {
      char fbuf[48];
      std::snprintf(fbuf, sizeof(fbuf), "FUEL %.0f HP %.0f", v->fuel, v->hp);
      platform.draw_text(240, 228, fbuf, 0.6f, 0.75f, 1.f);
    }
  }
  if (game.near_campfire()) {
    platform.draw_text(300, 32, "FIRE - R COOK", 1.f, 0.55f, 0.2f);
  }
  if (game.airdrop_alert() > 0.f) {
    platform.draw_text(160, 70, "AIRDROP INBOUND", 0.95f, 0.85f, 0.2f);
  }
  if (game.reload_cd() > 0.f) {
    platform.draw_text(200, 120, "RELOAD", 0.85f, 0.75f, 0.35f);
  }
  {
    const int ammo = game.ammo_for_equipped();
    if (ammo >= 0) {
      char abuf[32];
      std::snprintf(abuf, sizeof(abuf), "AMMO %d", ammo);
      platform.draw_text(240, 240, abuf, 0.85f, 0.85f, 0.55f);
    }
  }
  if (game.inventory().worn_count() > 0) {
    char wbuf[32];
    std::snprintf(wbuf, sizeof(wbuf), "GEAR %d", game.inventory().worn_count());
    platform.draw_text(380, 84, wbuf, 0.65f, 0.7f, 0.8f);
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
    platform.draw_text(300, 8, "BUNKER", 0.85f, 0.7f, 0.5f);
  } else {
    // Nearest bunker entrance bearing (simple compass)
    float best = 1e12f;
    float bx = 0.f, by = 0.f;
    bool found = false;
    for (int i = 0; i < game.mapgen().location_count(); ++i) {
      const MapLocation& L = game.mapgen().location(i);
      if (L.type != Loc_BunkerEntrance) continue;
      const float dx = L.pos.x - pl.pos.x;
      const float dy = L.pos.y - pl.pos.y;
      const float d2 = dx * dx + dy * dy;
      if (d2 < best) { best = d2; bx = dx; by = dy; found = true; }
    }
    if (found) {
      std::snprintf(buf, sizeof(buf), "BUNKER %.0f", std::sqrt(best));
      platform.draw_text(300, 8, buf, 0.7f, 0.75f, 0.55f);
      (void)bx; (void)by;
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
      if (L.type == Loc_BunkerEntrance) { r = 0.9f; g = 0.6f; b = 0.2f; }
      if (L.type == Loc_Military) { r = 0.3f; g = 0.55f; b = 0.3f; }
      if (L.type == Loc_City) { r = 0.7f; g = 0.55f; b = 0.25f; }
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
