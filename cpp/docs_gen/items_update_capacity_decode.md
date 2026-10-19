# Items_update / container capacity decode

Source: `original_docs/data.js` via `tools/decode_fn.js`, `tools/extract_item_array_cols.js`, `tools/extract_fn_onfunction.js`.

## Verified

| Topic | C2 evidence |
|--------|-------------|
| Master item stats | Runtime reads `E[20,536,119,false,null,[[0,<col>]]]` — Array **536**, row from item instance **119**, column index literal. |
| Container max (UI) | Column **301** used in `Items_update` capacity text: `(fill ? IV10 … : … E[20,536,119,false,null,[[0,301]]])` with `%` suffix on many item frames (decoded `Items_update_decoded.txt`). **11** read sites in `data.js`. |
| Fill counter | **IV10** on worn/GUI item (family **909**) paired with column 301 in the same AppendText templates. |
| Pickup space | `Check_space_pants` / `Check_item_pants` (and vest/outerwear/backpack analogs): **IV93 `[0..3]`** + region UID arrays **924/916/921/913** — `Contains` item id or empty `0`, then write count/id to paired Arrays. **No** `536/301` compare in `Check_space_pants.json`. |
| Equip swap | `Cut_arrays(n)` clears region Arrays when swapping clothing (`n` = 1 pants, 2 outerwear, 3 vest, 4 backpack); plays `tear_fabric` if container Array non-empty. |
| Hotbar → container | `init_item_activation` (7083 B body): param `"Backpack"` / `"Outerwear"` / `"Vest"` / `"Pants"` / `"Hands"` → `item_activate` + Function return **IV259** updates region Arrays (GUI). |
| GUI pick from worn | `Pick_player_item` called with `E[20,909,151]` from `Check_space_pants` failure branch (~144 KB On Function — drag/touch UI). |

## Not verified / blocked

| Topic | Reason |
|--------|--------|
| **“IV181” per-garment capacity** | Decoded `E[20,185,181,false,null]` is **not** `SetInstanceVar [10,181]` (zero `[10,181]` in `data.js`). Third numeric is part of Construct **object expression** (Array pick **185**), not an instance-var index on clothing. |
| **Numeric column 301 per item id** | Items Array **536** is not populated in exported `data.js` (no layout instance, no bulk SetAt into col 301). Cannot extract Taloon vs school bag volumes without missing runtime table source. |
| **Wire `clothing_containers.cpp` to volume** | Would require verified `container_max` per worn piece + item **size** column (likely **533** reads in client_log templates — not extracted). |

## Port status

- Keep **4× IV93 sub-slots** + `stack_max` (`clothing_containers.cpp`) until column **301** (and item size column) are extracted.
- `Pick_player_item` / full `init_item_activation`: **partial desktop** — `pick_player_item()` moves IV93 sub-slot → bag (no drag); `init_item_activation` hotbar→container still **blocked**.
- Inventory panel HUD: `gui_inventory` / `gui_item_backpack` / clothing cards from GUI_layout layer 53; click focus + E / double-click pick.

## Tools / artifacts

- `tools/extract_item_array_cols.js` → `cpp/docs_gen/item_array_cols.json`
- `tools/extract_items_table_col301.js` → `cpp/docs_gen/clothing_capacity_col301.json` (0 SetAt hits)
- `cpp/docs_gen/functions/Items_update_decoded.txt`
- `cpp/docs_gen/functions/Cut_arrays_decoded.txt`
- `cpp/docs_gen/functions/init_item_activation_decoded.txt`
- `cpp/docs_gen/functions/Check_item_pants_decoded.txt`
