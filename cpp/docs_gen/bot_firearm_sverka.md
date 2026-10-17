# Bot firearm bullets / SFX (bandit827 obj827)

Sources: `original_docs/data.js` bandit827 IV11 fire events; `Bullets_dmg_deal_decoded.txt` (obj910 IV1=-15 vs obj827).

| C2 | Port |
|----|------|
| obj910 IV1 = -15 on bandit827 hits | `kBotBanditBulletIv1` (damage path uses existing bandit NPC kind) |
| m4_bullet_bot-sheet0 (type t214) | `Bullet::bandit_gun_sprite` → draw when sprite present |
| IV11 fire audio (tag m4/akm) | `bot_bandit_fire_sfx(iv14, iv11, rng)` |
| Camp IV14 `{1,2,4,5,10}` | `bot_bandit_iv11_for_iv14()` → `{8,4,5,16,7}` |

Reload SFX unchanged: `Reload_rifle` → `reload_rifle.ogg`.
