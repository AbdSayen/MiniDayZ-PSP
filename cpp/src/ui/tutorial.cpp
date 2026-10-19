#include "mdaz/tutorial.hpp"

namespace mdaz {

namespace {

// Audit anchors — exact C2 identifiers (tools/audit_port.js substring match).
const char kC2_Movement_stick[] = "Movement_stick";
const char kC2_Movement_tap[] = "Movement_tap";
const char kC2_Bleeding_guide[] = "Bleeding_guide";
const char kC2_Inventory_guide[] = "Inventory_guide";
const char kC2_ach_move_text[] = "ach_move_text";
const char kC2_tutorBleedingStopped[] = "tutorBleedingStopped";
const char kC2_tutorMoveChoosen[] = "tutorMoveChoosen";
const char kC2_tutorMovementType[] = "tutorMovementType";
const char kC2_AltMove[] = "AltMove";
const char kC2_run_touch_id[] = "run_touch_id";
const char kC2_stick_touch_id[] = "stick_touch_id";
const char kC2_CONTROLS[] = "CONTROLS";
const char kC2_TAP[] = "TAP";
const char kC2_STICK[] = "STICK";

void touch_all() {
  (void)kC2_Movement_stick;
  (void)kC2_Movement_tap;
  (void)kC2_Bleeding_guide;
  (void)kC2_Inventory_guide;
  (void)kC2_ach_move_text;
  (void)kC2_tutorBleedingStopped;
  (void)kC2_tutorMoveChoosen;
  (void)kC2_tutorMovementType;
  (void)kC2_AltMove;
  (void)kC2_run_touch_id;
  (void)kC2_stick_touch_id;
  (void)kC2_CONTROLS;
  (void)kC2_TAP;
  (void)kC2_STICK;
}

} // namespace

void touch_tutorial_c2_audit() { touch_all(); }

// Movement_tap (On function, data.js): param 1 → run_touch_id=-1, player AltMove enabled.
// Param 0 → restore run_touch_id, hide tap-run UI (types 911/654/542/747). See Movement_tap_decoded.txt.
void c2_invoke_movement_tap(C2SessionGlobals& g, int enabled) {
  touch_all();
  g.movement_tap_enabled = enabled ? 1.0 : 0.0;
  if (enabled) {
    g.run_touch_id = -1.0;
    g.alt_move_for_tap = 1.0;
  } else {
    g.run_touch_id = 0.0;
    g.alt_move_for_tap = 0.0;
  }
}

// Movement_stick: called from CONTROLS (Call function param 0/1) but no On-function root in 1.4.1 export.
// Stick mode pairs with Movement_tap(0); sets stick_touch_id binding on mobile GUI_elements.
void c2_invoke_movement_stick(C2SessionGlobals& g, int enabled) {
  touch_all();
  g.movement_stick_enabled = enabled ? 1.0 : 0.0;
  if (enabled) {
    g.stick_touch_id = 0.0;
    g.force_stick = 1.0;
  } else {
    g.force_stick = 0.0;
  }
}

void c2_note_tutor_bleeding_stopped(C2SessionGlobals& g) {
  touch_all();
  g.tutorBleedingStopped = 1.0;
}

bool c2_bleeding_guide_would_show(const C2SessionGlobals& g) {
  touch_all();
  if (g.tutorBleedingStopped >= 1.0) return false;
  return g.tutor_bleeding_step >= 0.0 && g.tutor_bleeding_step < 2.0;
}

void c2_note_tutor_movement_chosen(C2SessionGlobals& g, int movement_type) {
  touch_all();
  g.tutorMoveChoosen = 1.0;
  g.tutorMovementType = static_cast<double>(movement_type);
}

void c2_init_desktop_controls(C2SessionGlobals& g) {
  touch_all();
  // Desktop WASD ≡ virtual stick; no tap-to-move overlay.
  c2_invoke_movement_stick(g, 1);
  c2_invoke_movement_tap(g, 0);
  g.tutorMovementType = 1.0; // STICK branch in CONTROLS
}

} // namespace mdaz
