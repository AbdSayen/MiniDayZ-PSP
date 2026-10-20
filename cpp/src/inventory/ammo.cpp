#include "mdaz/ammo.hpp"

namespace mdaz {

int count_ammo_in_bag(const Inventory& inv, ItemId ammo_id) {
  if (ammo_id <= 0) return 0;
  int n = 0;
  for (int i = 0; i < inv.slot_count(); ++i) {
    const InvSlot& s = inv.slot(i);
    if (s.id == ammo_id) n += s.count;
  }
  return n;
}

int consume_ammo_from_bag(Inventory& inv, ItemId ammo_id, int need) {
  if (ammo_id <= 0 || need <= 0) return 0;
  int taken = 0;
  for (int i = 0; i < inv.slot_count() && taken < need; ++i) {
    InvSlot& s = inv.slot(i);
    if (s.id != ammo_id || s.count <= 0) continue;
    const int want = need - taken;
    const int chunk = s.count < want ? s.count : want;
    inv.remove_at(i, chunk);
    taken += chunk;
  }
  return taken;
}

int count_ammo_for_reload(const Inventory& inv, ItemId ammo_id) {
  return count_ammo_in_bag(inv, ammo_id);  // flat view already covers container arrays
}

int consume_ammo_for_reload(Inventory& inv, ItemId ammo_id, int need) {
  if (ammo_id <= 0 || need <= 0) return 0;
  int taken = consume_ammo_from_bag(inv, ammo_id, need);
  return taken;
}

} // namespace mdaz
