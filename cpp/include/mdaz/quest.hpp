#pragma once

namespace mdaz {

// Quest / tutorial objectives scaffold — C2 Tutorial_events + mission strings.
enum QuestId {
  Quest_None = 0,
  Quest_SurviveDay1,
  Quest_FindBunker,
  Quest_KillZeds10,
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
  bool tick(int days, int zeds_killed, bool near_bunker);
  const char* label() const;
};

} // namespace mdaz
