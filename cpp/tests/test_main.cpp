#include <cstdio>

int test_balance();
int test_survival();
int test_inventory();
int test_combat();
int test_save();
int test_bunker();
int test_craft();

int main() {
  int failed = 0;
  failed += test_balance();
  failed += test_survival();
  failed += test_inventory();
  failed += test_combat();
  failed += test_save();
  failed += test_bunker();
  failed += test_craft();
  if (failed == 0) {
    std::printf("ALL TESTS PASSED\n");
    return 0;
  }
  std::printf("%d TEST SUITE(S) FAILED\n", failed);
  return 1;
}
