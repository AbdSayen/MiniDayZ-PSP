#include "mdaz/game.hpp"

#define P(T) extern "C" unsigned sz_##T = (unsigned)sizeof(mdaz::T);

P(Game)
P(WorldTilemaps)
P(BunkerMap)
P(MapGenerator)
P(ZombieSystem)
P(LootSystem)
P(CharAnims)
P(LayoutSprites)
P(BanditSystem)
P(BulletSystem)
P(AnimalSystem)
P(VehicleSystem)
P(Inventory)
P(Player)
