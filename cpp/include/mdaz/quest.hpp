#pragma once

namespace mdaz {

// Quest / tutorial objectives scaffold — C2 Tutorial_events + mission strings.
enum QuestId {
  Quest_None = 0,
  Quest_SurviveDay1,
  Quest_FindBunker,
  Quest_KillZeds10,
  Quest_VisitHospital,
  Quest_KillBandits3,
  Quest_CatchFish2,
  Quest_VisitPolice,
  Quest_VisitFirestation,
  Quest_VisitChurch,
  Quest_VisitMilitary,
  Quest_VisitHelicrash,
  Quest_Count
};

struct QuestState {
  QuestId active;
  bool completed[Quest_Count];
  int progress;
  int target;

  QuestState();
  void reset();
  void start(QuestId q);
  // Call from game loop with current stats; returns true if just completed.
  bool tick(int days, int zeds_killed, bool near_bunker, bool near_hospital = false,
            int bandits_killed = 0, int fish_caught = 0, bool near_police = false,
            bool near_firestation = false, bool near_church = false,
            bool near_military = false, bool near_helicrash = false);
  // Next quest after completing current (chain).
  QuestId next_after(QuestId done) const;
  const char* label() const;
};

} // namespace mdaz
