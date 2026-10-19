#include <cstdio>

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

int main() {
  int failed = 0;
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
  if (failed == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST SUITE(S) FAILED\n", failed);
  return 1;
}
