#include "mdaz/quest.hpp"

namespace mdaz {

QuestState::QuestState() { reset(); }

void QuestState::reset() {
  active = Quest_None;
  progress = 0;
  target = 0;
  for (int i = 0; i < Quest_Count; ++i) completed[i] = false;
}

void QuestState::start(QuestId q) {
  active = q;
  progress = 0;
  switch (q) {
  case Quest_SurviveDay1: target = 1; break;
  case Quest_FindBunker: target = 1; break;
  case Quest_KillZeds10: target = 10; break;
  case Quest_VisitHospital: target = 1; break;
  case Quest_KillBandits3: target = 3; break;
  case Quest_CatchFish2: target = 2; break;
  case Quest_VisitPolice: target = 1; break;
  default: target = 0; break;
  }
}

QuestId QuestState::next_after(QuestId done) const {
  switch (done) {
  case Quest_KillZeds10: return Quest_VisitHospital;
  case Quest_VisitHospital: return Quest_FindBunker;
  case Quest_FindBunker: return Quest_KillBandits3;
  case Quest_KillBandits3: return Quest_CatchFish2;
  case Quest_CatchFish2: return Quest_VisitPolice;
  case Quest_VisitPolice: return Quest_SurviveDay1;
  default: return Quest_None;
  }
}

bool QuestState::tick(int days, int zeds_killed, bool near_bunker, bool near_hospital,
                      int bandits_killed, int fish_caught, bool near_police) {
  if (active == Quest_None) return false;
  bool done = false;
  switch (active) {
  case Quest_SurviveDay1:
    progress = days >= 2 ? 1 : 0; // survive into day 2
    done = days >= 2;
    break;
  case Quest_FindBunker:
    progress = near_bunker ? 1 : 0;
    done = near_bunker;
    break;
  case Quest_KillZeds10:
    progress = zeds_killed;
    if (progress > target) progress = target;
    done = zeds_killed >= target;
    break;
  case Quest_VisitHospital:
    progress = near_hospital ? 1 : 0;
    done = near_hospital;
    break;
  case Quest_KillBandits3:
    progress = bandits_killed;
    if (progress > target) progress = target;
    done = bandits_killed >= target;
    break;
  case Quest_CatchFish2:
    progress = fish_caught;
    if (progress > target) progress = target;
    done = fish_caught >= target;
    break;
  case Quest_VisitPolice:
    progress = near_police ? 1 : 0;
    done = near_police;
    break;
  default:
    break;
  }
  if (done && active > Quest_None && active < Quest_Count) {
    completed[active] = true;
    active = Quest_None;
    return true;
  }
  return false;
}

const char* QuestState::label() const {
  switch (active) {
  case Quest_SurviveDay1: return "SURVIVE TO DAY 2";
  case Quest_FindBunker: return "FIND BUNKER";
  case Quest_KillZeds10: return "KILL 10 ZEDS";
  case Quest_VisitHospital: return "FIND HOSPITAL";
  case Quest_KillBandits3: return "KILL 3 BANDITS";
  case Quest_CatchFish2: return "CATCH 2 FISH";
  case Quest_VisitPolice: return "FIND POLICE STATION";
  default: return "";
  }
}

} // namespace mdaz
