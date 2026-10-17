# dmg_hint vs Hit_effect (from `original_docs/data.js`)

## Hit_effect — **verified**

`cpp/docs_gen/functions/Hit_effect_decoded.txt`:

- `SetLayoutEffectParam("Hit_effect", 0, 100)`
- `Wait 0.2`
- `SetLayoutEffectParam("Hit_effect", 0, 0)`

Port: `Game::hit_effect_t_` red fullscreen flash on `apply_incoming_damage` (~0.2s peak + fade).

## dmg_hint — **verified (floating text, not hit sprites)**

`cpp/docs_gen/functions/dmg_hint_decoded.txt`:

- OnFunction `dmg_hint` with target UID param 0
- Spawns obj529 (`Rain` layer) with Text `-{damage}` toward player↔target
- Param 2==1 → red 18px font (crit-style branch)

Not the same as `test_bullet_hit` (obj308) impact sprites.

**Port (2026-10-17):** `DmgHintSystem` in `combat.hpp` / `combat.cpp`; `Game` spawns on NPC hurt + player `apply_incoming_damage`; rendered in world space. Crit-style (param2==1 / red) when bullet IV8 second-hit exceeds rolled damage.

## Bullet IV2 — **partial**

`New_bullet_decoded.txt` / `Range_less_power`: IV2 3→2 @ 100px travel, 2→1 @ 220px (penetration tiers for obstacles).

Separate Bullets_dmg_deal branches use IV2 1/2/3 vs player hit zones — not ported (needs C2 collision geometry).
