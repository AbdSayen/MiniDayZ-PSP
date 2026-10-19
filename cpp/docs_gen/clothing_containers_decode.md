# Clothing container decode (Check_*_pants/vest/outerwear/backpack)

Source: `original_docs/data.js` via `tools/extract_fn_trigger.js` + `tools/decode_fn.js`.

## Verified (C2 event order)

| Function | Player gate IV | If gate `<= 0` → |
|----------|----------------|------------------|
| `Check_ammo_pants` / `Check_space_pants` | **IV19** | `Check_*_outerwear` |
| `Check_ammo_outerwear` / `Check_space_outerwear` | **IV17** | `Check_*_vest` |
| `Check_ammo_vest` / `Check_space_vest` | **IV11** | `Check_*_backpack` |
| `Check_ammo_backpack` / `Check_space_backpack` | **IV9** | fail / log (ammo) |

- Pickup: `Pick_inventory_item` → `Check_space_inventory` (bag) → cascade above (`Check_space_pants` …).
- Reload ammo: `Check_ammo_inventory` (bag) → same cascade (`Check_ammo_pants` …) on `Mag_reload` / Tap_reload paths.
- Each `Check_*` call passes Function object **IV93** subscripts **`[0,0]` … `[0,3]`** (four container sub-slots).
- Successful pickup writes **IV93 `[0,1]` = count**, **`[0,2]` = item id** (Array `SetX` in decode).

## Approximate in port

- **Region gates**: C2 sets IV19/17/11/9 to **1** when equipping container clothing (`Items_update` / wear events). Port sets gate from **worn item id → region** heuristic (`clothing_region_for_worn_item`).
- **Per-item cargo volume**: C2 UI uses items Array **536** column **301** (max) vs worn item **IV10** (fill) in `Items_update` — see `items_update_capacity_decode.md`. **Not wired:** table **536** not present in exported `data.js`; decoded “IV181” is an Array expression token, not instance var 181.
- Port still uses **4 IV93 sub-slots** per region with normal `stack_max` until column 301 (+ item size column) are extracted.

## Blocked / not ported

- GUI `Pick_player_item` / clothing-panel drag-drop.
- `init_item_activation` / `Items_update` sprite refresh when consuming from containers.
- Exact UID array **924/921/913** pick chains (Construct Array objects tied to player IV20/12/10).

## Port files

- `cpp/include/mdaz/clothing_containers.hpp`, `cpp/src/inventory/clothing_containers.cpp`
- `cpp/include/mdaz/ammo.hpp` — `count_ammo_for_reload` / `consume_ammo_for_reload`
- `cpp/src/inventory/inventory_ui.cpp` — `pick_inventory_item` cascade
- `cpp/src/core/game.cpp` — `try_mag_reload`, `ammo_for_equipped`
- Save **MDAZ v17** — `cloth_ids` / `cloth_counts[16]`
