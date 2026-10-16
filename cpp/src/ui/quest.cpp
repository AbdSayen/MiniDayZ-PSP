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
  default: target = 0; break;
  }
}

bool QuestState::tick(int days, int zeds_killed, bool near_bunker) {
  if (active == Quest_None) return false;
  bool done = false;
  switch (active) {
  case Quest_SurviveDay1:
    progress = days >= 1 ? 1 : 0;
    done = days >= 1;
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
  case Quest_SurviveDay1: return "SURVIVE DAY 1";
  case Quest_FindBunker: return "FIND BUNKER";
  case Quest_KillZeds10: return "KILL 10 ZEDS";
  default: return "";
  }
}

} // namespace mdaz
