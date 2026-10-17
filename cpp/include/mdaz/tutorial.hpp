#pragma once

#include "mdaz/c2_session_globals.hpp"

namespace mdaz {

// C2 tutorial / mobile controls (Mini DAYZ 1.4.1). Desktop skips overlay UI;
// flags and movement mode mirror data.js for audit parity.
void touch_tutorial_c2_audit();

// CONTROLS: TAP → Movement_stick(0)+Movement_tap(1); STICK → Movement_stick(1)+Movement_tap(0).
void c2_invoke_movement_stick(C2SessionGlobals& g, int enabled);
void c2_invoke_movement_tap(C2SessionGlobals& g, int enabled);

// Bleeding_guide suppressed after first bandage stop (tutorBleedingStopped=1 in C2).
void c2_note_tutor_bleeding_stopped(C2SessionGlobals& g);
bool c2_bleeding_guide_would_show(const C2SessionGlobals& g);

// tutorMoveChoosen set when movement tutorial choice is committed (C2 menu_click path).
void c2_note_tutor_movement_chosen(C2SessionGlobals& g, int movement_type);

void c2_init_desktop_controls(C2SessionGlobals& g);

} // namespace mdaz
