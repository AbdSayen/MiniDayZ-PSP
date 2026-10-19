# Visual fidelity batch (HUD font / clothing / minimap)

## Assets in `original_docs/images`

| Feature | Found | Missing |
|---------|-------|---------|
| HUD bitmap font | — | `bitmap_font*.png` (not in mirror; 5×7 rect fallback) |
| Minimap | `minimap_tile-sheet0/1`, `minimap_mark-sheet0`, `minimap_player-sheet0`, `minimap_hint-sheet0/1/2`, `minimap_dragger-sheet0`, `red_zone_minimap-sheet0` | C2 runtime minimap tilemap (generate_minimap) |
| Worn body overlays | Per-item sheets: `helmet_army`, `gorka_helmet`, `vest`, `molle_vest`, `gorka_jacket`, `gasmask`, … (via `item_sprites.cpp`) | `player_helmets` / `player_vests` object PNGs (C2 containers, no sheets in mirror) |

## Code

- `cpp/src/player/player_worn_visual.cpp` — layer pick + sprite base from item id
- `cpp/src/core/game.cpp` — `draw_player_worn_layers` after `draw_player_c2`
- `cpp/src/ui/hud.cpp` — tiled minimap, roads, POI sprites, layout player dot
- `cpp/tests/test_player_visual.cpp`
