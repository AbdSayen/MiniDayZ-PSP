# bot_reloading (obj522) — sverka from `original_docs/data.js`

Source decode: `cpp/docs_gen/functions/bot_reloading_decoded.txt` (event group on obj522 when IV9==0).

| IV14 | Wait (s) | IV9 refill |
|------|----------|------------|
| 1 | 2 | 15 |
| 2 | 2 | 6 |
| 3 | 2 | 2 |
| 4 | 2 | 5 |
| 5 | 2 | 10 |
| 6–10 | 2 | 30 |
| 11 | 2.5 | 6 |
| 12 | 2 | 7 |
| 15 | 2 | 10 |
| 16 | 3 | 30 |
| 17 | 3 | 11 |
| 19 | 3 | 30 |
| 20 | 3 | 18 |

Also sets IV10=1 during wait, `Reload_rifle` SFX at bot when in player hear radius.

Port: `bot_mag_capacity_for_weapon_iv14`, `bot_reload_duration_sec`, `bot_tick_mag_reload`, `Game::update_bandit_guns`.
