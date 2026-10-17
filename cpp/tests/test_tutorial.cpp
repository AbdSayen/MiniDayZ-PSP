#include "mdaz/c2_session_globals.hpp"
#include "mdaz/tutorial.hpp"

#include <cstdio>

int test_tutorial() {
  std::printf("--- test_tutorial / movement / tutor flags ---\n");
  int fail = 0;

  mdaz::touch_tutorial_c2_audit();

  mdaz::C2SessionGlobals g;
  mdaz::c2_init_desktop_controls(g);
  if (g.movement_stick_enabled != 1.0 || g.movement_tap_enabled != 0.0) {
    std::printf("FAIL desktop controls stick=%g tap=%g\n", g.movement_stick_enabled,
                g.movement_tap_enabled);
    ++fail;
  } else {
    std::printf("OK   CONTROLS STICK desktop defaults\n");
  }

  mdaz::c2_invoke_movement_tap(g, 1);
  if (g.run_touch_id != -1.0 || g.movement_tap_enabled != 1.0) {
    std::printf("FAIL Movement_tap(1) run_touch_id=%g\n", g.run_touch_id);
    ++fail;
  } else {
    std::printf("OK   Movement_tap AltMove/run_touch_id\n");
  }

  mdaz::c2_invoke_movement_stick(g, 0);
  mdaz::c2_invoke_movement_tap(g, 0);
  mdaz::c2_invoke_movement_stick(g, 1);
  if (g.movement_stick_enabled != 1.0) {
    std::printf("FAIL Movement_stick(1)\n");
    ++fail;
  } else {
    std::printf("OK   Movement_stick enable\n");
  }

  g.tutor_bleeding_step = 1.0;
  g.tutorBleedingStopped = 0.0;
  if (!mdaz::c2_bleeding_guide_would_show(g)) {
    std::printf("FAIL Bleeding_guide step 1 should show\n");
    ++fail;
  } else {
    std::printf("OK   Bleeding_guide gate step\n");
  }

  mdaz::c2_note_tutor_bleeding_stopped(g);
  if (g.tutorBleedingStopped != 1.0 || mdaz::c2_bleeding_guide_would_show(g)) {
    std::printf("FAIL tutorBleedingStopped suppress\n");
    ++fail;
  } else {
    std::printf("OK   tutorBleedingStopped\n");
  }

  mdaz::c2_note_tutor_movement_chosen(g, 0);
  if (g.tutorMoveChoosen != 1.0 || g.tutorMovementType != 0.0) {
    std::printf("FAIL tutorMoveChoosen/type\n");
    ++fail;
  } else {
    std::printf("OK   tutorMoveChoosen / tutorMovementType TAP\n");
  }

  return fail;
}
