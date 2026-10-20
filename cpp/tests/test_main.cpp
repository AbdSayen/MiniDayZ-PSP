#include <cstdio>
#include <cstring>

int test_balance();
int test_survival();
int test_inventory();
int test_combat();
int test_save();
int test_bunker();
int test_craft();
int test_systems();
int test_weather();
int test_weapon_unlocks();
int test_spawn_global();
int test_spawn_params();
int test_c2_session_globals();
int test_tutorial();
int test_map_fidelity();
int test_player_visual();
int test_selection();
int test_perks();
int test_car_hitbox();
int test_queue_q1();
int test_fidelity2();
int test_audio();
int test_pump();
int test_chop();
int test_trees();
int test_melee();
int test_fidelity2_more();

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  // "pump_last": run test_pump after all suites (order-independence check).
  const bool pump_last = argc > 1 && std::strcmp(argv[1], "pump_last") == 0;
  int failed = 0;
  failed += test_audio();
  if (!pump_last) failed += test_pump();
  failed += test_chop();
  failed += test_trees();
  failed += test_melee();
  failed += test_balance();
  failed += test_weather();
  failed += test_survival();
  failed += test_inventory();
  failed += test_combat();
  failed += test_save();
  failed += test_bunker();
  failed += test_craft();
  failed += test_systems();
  failed += test_weapon_unlocks();
  failed += test_spawn_global();
  failed += test_spawn_params();
  failed += test_c2_session_globals();
  failed += test_tutorial();
  failed += test_map_fidelity();
  failed += test_player_visual();
  failed += test_selection();
  failed += test_perks();
  failed += test_car_hitbox();
  failed += test_queue_q1();
  failed += test_fidelity2();
  failed += test_fidelity2_more();
  if (pump_last) failed += test_pump();
  if (failed == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST SUITE(S) FAILED\n", failed);
  return 1;
}
