# Аудит звука: data.js → порт (2026-10-20)

Источник: все действия плагина Audio (t178) в data.js. Их выгружали скрипты agent-tools/aq3–aq7.js: 261 уникальная строка без оружия плюс таблицы стрельбы. Семантику я сверил по c2runtime.js.

## Семантика c2runtime (реализована в src/platform/audio_mixer.cpp)
- `dbToLinear = 10^(db/20)` с ограничением [0,1]: +10 и +25 дБ играют на 1.0.
- Теги сравниваются без учёта регистра. Пустой тег означает «последний звук» (lastAudio).
- Пока включён SetSilent, Play* ничего не делает. Stop(tag) останавливает все экземпляры, StopAll — вообще всё.
- SetPaused: 0 = пауза, 1 = продолжить. IsTagPlaying не учитывает приостановленные.
- Позиционный звук: inverse-модель, ref 600, max 10000, rolloff 10, слушатель на Z 600 (свойства Audio [0,0,0,1,1,600,600,10000,10]). Слушатель — player_base, `set_listener` вызывается каждый кадр.
- Позиционный зацикленный звук останавливается, когда уничтожен его объект (огонь, вспышка, машина).

## События и их место в порту

| C2 событие (sid) | Порт |
|---|---|
| Game_events OnLayoutStart: StopAll; Sounds_on==0 → SetSilent | `audio_layout_start()` (новая игра, загрузка, туториал) |
| Day&Night 650766536843728 / 607253053633264: TriggerOnce по часам, inBunker==0, loop 0 дБ, теги day_loop / Night_loop, каждый гасит другой | `update_audio` |
| Bunker_generate: Stop rain_loop / rain_roof / day_loop / night_loop + bunker_door | `audio_bunker_enter()` |
| Bunker_erase 172106691563512 / 427082255021896 | `audio_bunker_exit()` |
| Дождь 609331584746158: rain_loop (blizzard при rains_level 2) loop −5 [rain_loop] + rainroof loop −5 [rain_roof]; пауза внутри/снаружи по t337; стоп 448001804847237 | `update_audio` |
| House_transparenting каждые 0,1 с (t928 + Rain): та же перестановка пауз | `update_audio` |
| Menu_init: ambient loop −10 дБ (все переходы в меню начинаются со StopAll) | `audio_menu_start()` |
| Animations: шаги каждые 0,25 с, −5 дБ, [run] | `audio_footsteps` |
| Firearm shooting (игрок, IV1 / IV43) | `c2_player_fire_sfx` |
| Bandit t827 IV11 / friend t522 IV14 | `c2_bandit_fire_sfx` / `c2_friend_fire_sfx` |
| Turret OnTargetAcquired зеда: spotted_0..6 [zmb] (< Player_Hear_radius) | `update_audio` |
| Zed_common / tank dmg_deal: attack_0, 1, 10–13 [zmb] | `Zombie::hit_tick` |
| Screamer каждые 2,5 с, если !IsTagPlaying: attack_8 −10 [screamer] | `update_audio` |
| Закопанный зед в пределах 200 px каждые 3 с: deerrun (164081998282255) | `update_audio` |
| Волк/собака: wolf_spot*, wolf_attack* + body −5 у игрока | `update_audio` |
| crow_fly: Wait random(2) → crow_1..5 | `Animal::flew_evt` |
| Mag_reload: reload_pistol −5, Wait, Reload_rifle −5 | `try_mag_reload` + `sfx_later` |
| Pick_player_item / Check_space / Drop_* | `c2_pickup_sfx` / `c2_drop_sfx` |
| item_activate / item_activate2 | `c2_item_activate_sfx` / `_2_sfx` |
| Костёр: fireplace_loop loop −5 ["fireplaceloop_"&UID]; Stop по таймеру Burn | `update_audio` (тег по слоту) |
| flare_burn loop −5 [Flare] | `update_audio` |
| Машина: door_open, Wait 0,7, door_close; engine_start, Wait 0,7, engine_idle loop −10 ["<uaz/volga>_engine_"&UID]; rate 1+abs(Speed/Max); при выходе Stop + door_open + engine_stop; поломка → car_explode + Stop + engine_stop | `update_audio` |
| Удары по дереву chop*, падение treefall −5; car_crash*; Fence_destroy*; trap_bear_0; f1_explode +10; bottle_break_0; punch_swipes_0 +10; gren pin | места вызова в game.cpp |
| Tutorial: MD_Perk_1, Ak74_1 [m4] ×3, tutor_death [tutor] | game_tutorial.cpp |

## Удалённые выдуманные звуки
Звуки крафта, квестов, аирдропа, смерти, рыбалки, сбора урожая, ветра, грома, воя, кашля, одиночные звуки дождя, NVG/батареи/противогаза/морфина/набора для чистки/шитья, предупреждения о топливе, «deer» при выстреле, общий «hit», случайные крики ворон. Ни у одного из этих мест нет действия Audio в data.js.
